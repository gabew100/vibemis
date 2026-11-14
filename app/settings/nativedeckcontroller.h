#pragma once

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStringList>

namespace NativeDeckController {

inline constexpr char SettingKey[] = "nativedeckcontrols";

inline bool isAvailable(const QDir& hidrawRoot = QDir(QStringLiteral("/sys/class/hidraw")),
                        const QDir& deviceRoot = QDir(QStringLiteral("/dev")))
{
    for (const QString& name : hidrawRoot.entryList({QStringLiteral("hidraw*")}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFile uevent(hidrawRoot.filePath(name + QStringLiteral("/device/uevent")));
        if (!uevent.open(QIODevice::ReadOnly)) {
            continue;
        }
        for (const QByteArray& rawLine : uevent.readAll().split('\n')) {
            const QByteArray line = rawLine.trimmed();
            if (!line.startsWith("HID_ID=")) {
                continue;
            }
            const QList<QByteArray> id = line.mid(7).split(':');
            if (id.size() == 3 && id[1].toUInt(nullptr, 16) == 0x28de &&
                    id[2].toUInt(nullptr, 16) == 0x1205) {
                QFile device(deviceRoot.filePath(name));
                if (device.open(QIODevice::ReadWrite | QIODevice::ExistingOnly)) {
                    return true;
                }
            }
        }
    }
    return false;
}

inline bool configure(const QSettings& settings,
                      const QDir& hidrawRoot = QDir(QStringLiteral("/sys/class/hidraw")),
                      const QDir& deviceRoot = QDir(QStringLiteral("/dev")))
{
    // An explicit launch override wins over the saved setting, including =0.
    const bool enabled = qEnvironmentVariableIsSet("VIBEMIS_NATIVE_DECK_GYRO")
            ? qgetenv("VIBEMIS_NATIVE_DECK_GYRO") == "1"
            : settings.value(SettingKey, false).toBool();
    if (!enabled || !isAvailable(hidrawRoot, deviceRoot)) {
        return false;
    }

    QStringList filtered;
    for (const QString& id : QString::fromUtf8(qgetenv("SDL_GAMECONTROLLER_IGNORE_DEVICES")).split(',')) {
        const QString normalized = id.trimmed().toLower();
        if (!normalized.isEmpty() && normalized != QStringLiteral("0x28de/0x1205") &&
                normalized != QStringLiteral("0x28de/0x11ff")) {
            filtered.append(id);
        }
    }
    filtered.append(QStringLiteral("0x28de/0x11ff"));
    qputenv("SDL_GAMECONTROLLER_IGNORE_DEVICES", filtered.join(',').toUtf8());
    if (!qEnvironmentVariableIsEmpty("SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT")) {
        qputenv("SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT", "0x28de/0x1205");
    }
    qputenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD", "0");
    qputenv("SDL_JOYSTICK_HIDAPI", "1");
    qputenv("SDL_JOYSTICK_HIDAPI_STEAMDECK", "1");
    return true;
}

} // namespace NativeDeckController
