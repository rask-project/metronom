#include "AudioOutputInfo.h"

#ifdef Q_OS_MACOS
#include <CoreAudio/CoreAudio.h>
#include <QString>
#include <algorithm>
#include <bit>
#include <cstdint>
#include <memory>
#include <vector>
#elifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#endif

#ifdef Q_OS_MACOS

namespace
{
// kIOAudioOutputPortSubTypeInternalSpeaker: the built-in device's data source when it plays on the speaker.
constexpr UInt32 kInternalSpeakerSource = 'ispk';

struct CFDeleter {
    void operator()(CFTypeRef ref) const noexcept { CFRelease(ref); }
};

AudioObjectPropertyAddress address(AudioObjectPropertySelector selector, AudioObjectPropertyScope scope) noexcept
{
    return {.mSelector = selector, .mScope = scope, .mElement = kAudioObjectPropertyElementMain};
}

template <typename T>
std::optional<T> property(AudioObjectID object, AudioObjectPropertySelector selector, AudioObjectPropertyScope scope)
{
    const AudioObjectPropertyAddress propertyAddress = address(selector, scope);
    if (!AudioObjectHasProperty(object, &propertyAddress)) {
        return std::nullopt;
    }

    T value{};
    UInt32 size = sizeof(T);
    if (AudioObjectGetPropertyData(object, &propertyAddress, 0, nullptr, &size, &value) != noErr) {
        return std::nullopt;
    }
    return value;
}

std::vector<AudioObjectID>
objects(AudioObjectID object, AudioObjectPropertySelector selector, AudioObjectPropertyScope scope)
{
    const AudioObjectPropertyAddress propertyAddress = address(selector, scope);
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(object, &propertyAddress, 0, nullptr, &size) != noErr) {
        return {};
    }

    std::vector<AudioObjectID> result(size / sizeof(AudioObjectID));
    if (AudioObjectGetPropertyData(object, &propertyAddress, 0, nullptr, &size, result.data()) != noErr) {
        return {};
    }
    return result;
}

// Qt's device ids on macOS are the CoreAudio UIDs. The property is a CFStringRef the caller releases.
QByteArray deviceUid(AudioObjectID device)
{
    const AudioObjectPropertyAddress propertyAddress =
        address(kAudioDevicePropertyDeviceUID, kAudioObjectPropertyScopeGlobal);
    std::uintptr_t raw = 0;
    UInt32 size = sizeof(raw);
    if (AudioObjectGetPropertyData(device, &propertyAddress, 0, nullptr, &size, &raw) != noErr || raw == 0) {
        return {};
    }

    const auto uid = std::bit_cast<CFStringRef>(raw);
    const std::unique_ptr<const void, CFDeleter> owner(uid);
    return QString::fromCFString(uid).toUtf8();
}

std::vector<AudioObjectID> devices()
{
    return objects(kAudioObjectSystemObject, kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal);
}

bool isBuiltInSpeaker(AudioObjectID device)
{
    const auto transport = property<UInt32>(device, kAudioDevicePropertyTransportType, kAudioObjectPropertyScopeGlobal);
    if (transport != kAudioDeviceTransportTypeBuiltIn) {
        return false;
    }
    if (objects(device, kAudioDevicePropertyStreams, kAudioObjectPropertyScopeOutput).empty()) {
        return false;
    }
    // Macs with a headphone jack expose it as another built-in output.
    const auto source = property<UInt32>(device, kAudioDevicePropertyDataSource, kAudioObjectPropertyScopeOutput);
    return !source || *source == kInternalSpeakerSource;
}

UInt32 frames(AudioObjectID object, AudioObjectPropertySelector selector, AudioObjectPropertyScope scope)
{
    return property<UInt32>(object, selector, scope).value_or(0);
}
} // namespace

std::optional<QByteArray> AudioOutputInfo::builtInSpeakerId()
{
    const std::vector<AudioObjectID> all = devices();
    const auto speaker = std::ranges::find_if(all, isBuiltInSpeaker);
    if (speaker == all.end()) {
        return std::nullopt;
    }
    return deviceUid(*speaker);
}

// Device and stream latency, safety offset and IO buffer: what CoreAudio counts until a frame is heard.
std::optional<double> AudioOutputInfo::latencySeconds(const QByteArray &deviceId)
{
    const std::vector<AudioObjectID> all = devices();
    const auto device =
        std::ranges::find_if(all, [&deviceId](AudioObjectID candidate) { return deviceUid(candidate) == deviceId; });
    if (device == all.end()) {
        return std::nullopt;
    }

    const auto sampleRate =
        property<Float64>(*device, kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal);
    if (!sampleRate || *sampleRate <= 0.0) {
        return std::nullopt;
    }

    constexpr AudioObjectPropertyScope output = kAudioObjectPropertyScopeOutput;
    UInt32 total = frames(*device, kAudioDevicePropertyLatency, output) +
                   frames(*device, kAudioDevicePropertySafetyOffset, output) +
                   frames(*device, kAudioDevicePropertyBufferFrameSize, output);
    const std::vector<AudioObjectID> streams = objects(*device, kAudioDevicePropertyStreams, output);
    if (!streams.empty()) {
        total += frames(streams.front(), kAudioStreamPropertyLatency, kAudioObjectPropertyScopeGlobal);
    }
    return total / *sampleRate;
}

#elifdef Q_OS_ANDROID

std::optional<QByteArray> AudioOutputInfo::builtInSpeakerId()
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const QJniObject service =
        QJniObject::getStaticObjectField("android/content/Context", "AUDIO_SERVICE", "Ljava/lang/String;");
    const QJniObject manager = context.callObjectMethod(
        "getSystemService",
        "(Ljava/lang/String;)Ljava/lang/Object;",
        service.object<jstring>()
    );
    if (!manager.isValid()) {
        return std::nullopt;
    }

    const jint outputs = QJniObject::getStaticField<jint>("android/media/AudioManager", "GET_DEVICES_OUTPUTS");
    const jint speakerType = QJniObject::getStaticField<jint>("android/media/AudioDeviceInfo", "TYPE_BUILTIN_SPEAKER");
    const QJniObject list = manager.callObjectMethod("getDevices", "(I)[Landroid/media/AudioDeviceInfo;", outputs);
    if (!list.isValid()) {
        return std::nullopt;
    }

    // Qt's device ids on Android are AudioDeviceInfo.getId().
    QJniEnvironment env;
    const auto array = list.object<jobjectArray>();
    const jsize count = env->GetArrayLength(array);
    for (jsize i = 0; i < count; ++i) {
        const QJniObject device = QJniObject::fromLocalRef(env->GetObjectArrayElement(array, i));
        if (device.callMethod<jint>("getType") == speakerType) {
            return QByteArray::number(device.callMethod<jint>("getId"));
        }
    }
    return std::nullopt;
}

std::optional<double> AudioOutputInfo::latencySeconds(const QByteArray &)
{
    return std::nullopt;
}

#else

std::optional<QByteArray> AudioOutputInfo::builtInSpeakerId()
{
    return std::nullopt;
}

std::optional<double> AudioOutputInfo::latencySeconds(const QByteArray &)
{
    return std::nullopt;
}

#endif
