#pragma once

#include <QByteArray>
#include <optional>

// What the platform tells about the audio outputs, beyond Qt's device list.
namespace AudioOutputInfo
{
// The device's own loudspeaker, as a QAudioDevice id (macOS and Android).
std::optional<QByteArray> builtInSpeakerId();
// The output latency the system reports for a QAudioDevice id, in seconds (macOS).
std::optional<double> latencySeconds(const QByteArray &deviceId);
} // namespace AudioOutputInfo
