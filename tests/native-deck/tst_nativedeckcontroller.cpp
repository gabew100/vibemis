#include <QtTest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

#include "../../app/settings/nativedeckcontroller.h"
#include "../../app/settings/streamingpreferences.h"

// Display probes are unrelated to saving controller preferences.
namespace WMUtils {
bool isRunningWayland() { return false; }
bool isGpuSlow() { return false; }
}

class NativeDeckControllerTest : public QObject
{
    Q_OBJECT

private:
    const QList<QByteArray> variables = {"VIBEMIS_NATIVE_DECK_GYRO", "SDL_AUDIODRIVER"};
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

    static bool isAvailable(const QTemporaryDir& root)
    {
        return NativeDeckController::isAvailable(QDir(root.filePath("sys")), QDir(root.filePath("dev")));
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
        }
        original.clear();
        originallySet.clear();
    }

    void defaultIsOff()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
        QVERIFY(!NativeDeckController::isEnabled(settings));
    }

    void savedPreference_data()
    {
        QTest::addColumn<bool>("saved");
        QTest::newRow("enabled") << true;
        QTest::newRow("disabled") << false;
    }

    void savedPreference()
    {
        QFETCH(bool, saved);
        QTemporaryDir root;
        QVERIFY(root.isValid());
        {
            QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
            settings.setValue(NativeDeckController::SettingKey, saved);
        }
        QSettings reloaded(root.filePath("settings.ini"), QSettings::IniFormat);
        QCOMPARE(NativeDeckController::isEnabled(reloaded), saved);
    }

    void hardwareAvailability()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(!isAvailable(root));
        addDevice(root, "0003:000028DE:000011FF");
        QVERIFY(!isAvailable(root));
        addDevice(root);
        QVERIFY(isAvailable(root));
        QFile device(root.filePath("dev/hidraw0"));
        QVERIFY(device.open(QIODevice::ReadOnly));
        QCOMPARE(device.readAll(), QByteArray("untouched"));
        device.close();
        QVERIFY(QFile::remove(device.fileName()));
        QVERIFY(!isAvailable(root));
        QVERIFY(!QFile::exists(device.fileName()));
    }

    void followsSysfsDeviceSymlinks()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root);
        QVERIFY(QDir().rename(root.filePath("sys/hidraw0"), root.filePath("endpoint")));
        QVERIFY(QFile::link(root.filePath("endpoint"), root.filePath("sys/hidraw0")));
        QVERIFY(isAvailable(root));
    }

    void readOnlyHardwareIsUnavailable()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        addDevice(root);
        QVERIFY(QFile::setPermissions(root.filePath("dev/hidraw0"), QFileDevice::ReadOwner));
        QFile probe(root.filePath("dev/hidraw0"));
        if (probe.open(QIODevice::ReadWrite | QIODevice::ExistingOnly)) {
            QSKIP("Run as an unprivileged user to exercise device permissions");
        }
        QVERIFY(!isAvailable(root));
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

    void queryReadsSettingsWithoutGui()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert("XDG_CONFIG_HOME", root.path());
        environment.insert("QT_QPA_PLATFORM", "vibemis-no-platform-plugin");
        environment.remove("DISPLAY");
        environment.remove("WAYLAND_DISPLAY");
        environment.remove("VIBEMIS_NATIVE_DECK_GYRO");
        QProcess process;
        process.setProgram(QCoreApplication::applicationFilePath());
        process.setArguments({"--native-deck-controls-enabled"});
        process.setProcessEnvironment(environment);
        process.start();
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(process.readAllStandardOutput(), QByteArray("0\n"));

        writeFile(root.filePath("Vibemis Project/Vibemis.conf"), "[General]\nnativedeckcontrols=true\n");
        process.start();
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(process.readAllStandardOutput(), QByteArray("1\n"));

        environment.insert("VIBEMIS_NATIVE_DECK_GYRO", "0");
        process.setProcessEnvironment(environment);
        process.start();
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(process.readAllStandardOutput(), QByteArray("0\n"));
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
        QSettings settings(root.filePath("settings.ini"), QSettings::IniFormat);
        settings.setValue(NativeDeckController::SettingKey, saved);
        qputenv("VIBEMIS_NATIVE_DECK_GYRO", overrideValue);
        QCOMPARE(NativeDeckController::isEnabled(settings), expected);
        QCOMPARE(settings.value(NativeDeckController::SettingKey).toBool(), saved);
    }
};

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("Vibemis Project");
    QCoreApplication::setApplicationName("Vibemis");
    const int queryResult = NativeDeckController::queryPreference(argc, argv, QSettings());
    if (queryResult >= 0) {
        return queryResult;
    }
    QCoreApplication application(argc, argv);
    NativeDeckControllerTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_nativedeckcontroller.moc"
