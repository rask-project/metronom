#pragma once

#include <QAudioDevice>
#include <QByteArray>
#include <QList>
#include <QString>
#include <algorithm>
#include <iterator>
#include <optional>

namespace AudioDevices
{
inline std::optional<int> indexOf(const QList<QAudioDevice> &devices, const QByteArray &id)
{
    const auto it = std::ranges::find(devices, id, &QAudioDevice::id);
    return it == devices.end() ? std::nullopt : std::optional{static_cast<int>(std::distance(devices.begin(), it))};
}

inline int defaultIndex(const QList<QAudioDevice> &devices)
{
    const auto it = std::ranges::find_if(devices, &QAudioDevice::isDefault);
    return it == devices.end() ? 0 : static_cast<int>(std::distance(devices.begin(), it));
}

// The device the user chose, while it exists; the system default otherwise.
inline int chosenOrDefaultIndex(const QList<QAudioDevice> &devices, const QByteArray &chosenId)
{
    return indexOf(devices, chosenId).value_or(defaultIndex(devices));
}

inline QList<QString> names(const QList<QAudioDevice> &devices)
{
    QList<QString> result;
    result.reserve(devices.size());
    std::ranges::transform(devices, std::back_inserter(result), &QAudioDevice::description);
    return result;
}
} // namespace AudioDevices
