#pragma once

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStringList>
#include <cstdio>

namespace NativeDeckController {

inline constexpr char SettingKey[] = "nativedeckcontrols";

inline bool isEnabled(const QSettings& settings)
{
    return qEnvironmentVariableIsSet("VIBEMIS_NATIVE_DECK_GYRO")
            ? qgetenv("VIBEMIS_NATIVE_DECK_GYRO") == "1"
            : settings.value(SettingKey, false).toBool();
}

inline int queryPreference(int argc, char* const argv[], const QSettings& settings)
{
    if (argc != 2 || qstrcmp(argv[1], "--native-deck-controls-enabled") != 0) {
        return -1;
    }
    // AppRun reads only this bit; do not initialize SDL or a Qt platform plugin.
    if (std::fputs(isEnabled(settings) ? "1\n" : "0\n", stdout) == EOF) {
        return 1;
    }
    return std::fflush(stdout) == 0 ? 0 : 1;
}

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

} // namespace NativeDeckController
