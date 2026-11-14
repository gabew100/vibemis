#include <QtTest>
#include <QTemporaryDir>

#include "../../app/settings/nativedeckcontroller.h"
#include "../../app/settings/streamingpreferences.h"

// Keep platform display probes deterministic while exercising the real preferences.
namespace WMUtils {
bool isRunningWayland() { return false; }
bool isGpuSlow() { return false; }
}

class NativeDeckControllerTest : public QObject
{
    Q_OBJECT

private:
    const QList<QByteArray> variables = {
        "VIBEMIS_NATIVE_DECK_GYRO", "SDL_GAMECONTROLLER_IGNORE_DEVICES",
        "SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT", "SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD",
        "SDL_JOYSTICK_HIDAPI", "SDL_JOYSTICK_HIDAPI_STEAMDECK", "SDL_AUDIODRIVER"
    };
    QMap<QByteArray, QByteArray> original;
    QSet<QByteArray> originallySet;

    static void writeFile(const QString& path, const QByteArray& contents)
    {
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(contents), contents.size());
    }

    static void addDevice(const QTemporaryDir& root, const QByteArray& id = "0003:000028DE:00001205")
    {
        writeFile(root.filePath("sys/hidraw0/device/uevent"), "DRIVER=hid-steam\nHID_ID=" + id + "\n");
        writeFile(root.filePath("dev/hidraw0"), "untouched");
    }

    static bool configure(const QTemporaryDir& root, const QSettings& settings)
    {
        return NativeDeckController::configure(settings, QDir(root.filePath("sys")), QDir(root.filePath("dev")));
    }

private slots:
    void initTestCase()
    {
        SDL_version version;
        SDL_GetVersion(&version);
        qInfo("SDL runtime: %u.%u.%u", static_cast<unsigned>(version.major),
              static_cast<unsigned>(version.minor), static_cast<unsigned>(version.patch));
    }

    void init()
    {
        for (const QByteArray& name : variables) {
            if (qEnvironmentVariableIsSet(name.constData())) {
                originallySet.insert(name);
                original[name] = qgetenv(name.constData());
            }
            qunsetenv(name.constData());
        }
    }

    void cleanup()
    {
        for (const QByteArray& name : variables) {
            if (originallySet.contains(name)) {
                qputenv(name.constData(), original.value(name));
            } else {
                qunsetenv(name.constData());
            }
            SDL_ResetHint(name.constData());
        }
        original.clear();
        originallySet.clear();
    }

    void defaultPreservesSteamInput()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root);
        QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
        qputenv("SDL_GAMECONTROLLER_IGNORE_DEVICES", "0x28de/0x1205,0x054c/0x09cc");
        qputenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD", "1");
        QVERIFY(!configure(root, settings));
        QCOMPARE(qgetenv("SDL_GAMECONTROLLER_IGNORE_DEVICES"), QByteArray("0x28de/0x1205,0x054c/0x09cc"));
        QCOMPARE(qgetenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD"), QByteArray("1"));
        QVERIFY(!qEnvironmentVariableIsSet("SDL_JOYSTICK_HIDAPI_STEAMDECK"));
    }

    void savedPreferenceSurvivesRestart()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root);
        {
            QSettings saved(root.filePath("settings.ini"), QSettings::IniFormat);
            saved.setValue(NativeDeckController::SettingKey, true);
        }
        QSettings reloaded(root.filePath("settings.ini"), QSettings::IniFormat);
        QVERIFY(configure(root, reloaded));
        QCOMPARE(qgetenv("SDL_GAMECONTROLLER_IGNORE_DEVICES"), QByteArray("0x28de/0x11ff"));
        QCOMPARE(qgetenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD"), QByteArray("0"));
        QCOMPARE(qgetenv("SDL_JOYSTICK_HIDAPI"), QByteArray("1"));
        QCOMPARE(qgetenv("SDL_JOYSTICK_HIDAPI_STEAMDECK"), QByteArray("1"));
        QVERIFY(!qEnvironmentVariableIsSet("SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT"));
        QFile device(root.filePath("dev/hidraw0"));
        QVERIFY(device.open(QIODevice::ReadOnly));
        QCOMPARE(device.readAll(), QByteArray("untouched"));
    }

    void missingOrUnrelatedHardwarePreservesEnvironment()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
        settings.setValue(NativeDeckController::SettingKey, true);
        qputenv("SDL_GAMECONTROLLER_IGNORE_DEVICES", "0x28de/0x1205");
        QVERIFY(!configure(root, settings));
        addDevice(root, "0003:000028DE:000011FF");
        QVERIFY(!configure(root, settings));
        QCOMPARE(qgetenv("SDL_GAMECONTROLLER_IGNORE_DEVICES"), QByteArray("0x28de/0x1205"));
        QVERIFY(!qEnvironmentVariableIsSet("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD"));
        addDevice(root);
        QVERIFY(QFile::remove(root.filePath("dev/hidraw0")));
        QVERIFY(!configure(root, settings));
        QVERIFY(!QFile::exists(root.filePath("dev/hidraw0")));
    }

    void filtersAreScopedAndIdempotent()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root, "0003:000028de:00001205");
        QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
        settings.setValue(NativeDeckController::SettingKey, true);
        qputenv("SDL_GAMECONTROLLER_IGNORE_DEVICES", "0x28de/0x1205,0x054c/0x09cc, 0x28DE/0x1205,0x28de/0x11ff,");
        qputenv("SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT", "0x28de/0x11ff");
        QVERIFY(configure(root, settings));
        const QByteArray expected("0x054c/0x09cc,0x28de/0x11ff");
        QCOMPARE(qgetenv("SDL_GAMECONTROLLER_IGNORE_DEVICES"), expected);
        QCOMPARE(qgetenv("SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT"), QByteArray("0x28de/0x1205"));
        QVERIFY(configure(root, settings));
        QCOMPARE(qgetenv("SDL_GAMECONTROLLER_IGNORE_DEVICES"), expected);
    }

    void followsSysfsDeviceSymlinks()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root);
        QVERIFY(QDir().rename(root.filePath("sys/hidraw0"), root.filePath("endpoint")));
        QVERIFY(QFile::link(root.filePath("endpoint"), root.filePath("sys/hidraw0")));
        QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
        settings.setValue(NativeDeckController::SettingKey, true);
        QVERIFY(configure(root, settings));
    }

    void readOnlyHardwarePreservesEnvironment()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root);
        QVERIFY(QFile::setPermissions(root.filePath("dev/hidraw0"), QFileDevice::ReadOwner));
        QFile probe(root.filePath("dev/hidraw0"));
        if (probe.open(QIODevice::ReadWrite | QIODevice::ExistingOnly)) {
            QSKIP("Run as an unprivileged user to exercise device permissions");
        }
        QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
        settings.setValue(NativeDeckController::SettingKey, true);
        QVERIFY(!configure(root, settings));
        QVERIFY(!qEnvironmentVariableIsSet("SDL_GAMECONTROLLER_IGNORE_DEVICES"));
    }

    void savedDisabledPreference()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root);
        {
            QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
            settings.setValue(NativeDeckController::SettingKey, true);
            settings.setValue(NativeDeckController::SettingKey, false);
        }
        QSettings reloaded(root.filePath("settings.ini"), QSettings::IniFormat);
        QVERIFY(!configure(root, reloaded));
        QVERIFY(!qEnvironmentVariableIsSet("SDL_JOYSTICK_HIDAPI_STEAMDECK"));
    }

    void applicationPreferenceSaveAndReload()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QCoreApplication::setOrganizationName("Vibemis native Deck tests");
        QCoreApplication::setApplicationName("Preferences");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, root.path());
        qputenv("SDL_AUDIODRIVER", "dummy");
        {
            QScopedPointer<StreamingPreferences> preferences(StreamingPreferences::createDetached());
            QVERIFY(!preferences->nativeDeckControls);
            QVERIFY(preferences->setProperty("nativeDeckControls", true));
            preferences->save();
        }
        {
            QScopedPointer<StreamingPreferences> reloaded(StreamingPreferences::createDetached());
            QVERIFY(reloaded->property("nativeDeckControls").toBool());
            QVERIFY(QSettings().value(NativeDeckController::SettingKey).toBool());
            reloaded->nativeDeckControls = false;
            reloaded->save();
        }
        QScopedPointer<StreamingPreferences> disabled(StreamingPreferences::createDetached());
        QVERIFY(!disabled->nativeDeckControls);
    }

    void updatesRegisteredSdlHintCallbacks()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root);
        QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
        settings.setValue(NativeDeckController::SettingKey, true);
        qputenv("SDL_GAMECONTROLLER_IGNORE_DEVICES", "0x28de/0x1205");
        SDL_ResetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES);
        QByteArray observed;
        const auto callback = [](void* userdata, const char*, const char*, const char* value) {
            *static_cast<QByteArray*>(userdata) = value;
        };
        SDL_AddHintCallback(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES, callback, &observed);
        const bool configured = configure(root, settings);
        SDL_DelHintCallback(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES, callback, &observed);
        QVERIFY(configured);
        QCOMPARE(observed, QByteArray("0x28de/0x11ff"));
        QCOMPARE(QByteArray(SDL_GetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES)), QByteArray("0x28de/0x11ff"));
    }

    void launchOverride_data()
    {
        QTest::addColumn<bool>("saved");
        QTest::addColumn<QByteArray>("overrideValue");
        QTest::addColumn<bool>("expected");
        QTest::newRow("enable-without-saved-setting") << false << QByteArray("1") << true;
        QTest::newRow("disable-saved-setting") << true << QByteArray("0") << false;
        QTest::newRow("empty-override") << true << QByteArray("") << false;
    }

    void launchOverride()
    {
        QFETCH(bool, saved);
        QFETCH(QByteArray, overrideValue);
        QFETCH(bool, expected);
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root);
        QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
        settings.setValue(NativeDeckController::SettingKey, saved);
        qputenv("VIBEMIS_NATIVE_DECK_GYRO", overrideValue);
        QCOMPARE(configure(root, settings), expected);
        QCOMPARE(settings.value(NativeDeckController::SettingKey).toBool(), saved);
    }
};

QTEST_GUILESS_MAIN(NativeDeckControllerTest)
#include "tst_nativedeckcontroller.moc"
