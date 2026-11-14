# Native Steam Deck motion forwarding (AppImage)

In Vibemis, open **Settings > Input & gamepad > Gamepad Settings**:

1. Enable **Use native Steam Deck controls (restart required)**.
2. Enable **Forward motion controls (gyro)**.
3. Quit and relaunch Vibemis from Steam Game Mode.

No Steam launch option is needed. The native-controls toggle appears in the
AppImage when the Deck's native hidraw endpoint is readable and writable. It also
remains available if a saved native-controls preference needs to be turned off.
The preference defaults to off. AppRun reads it through a headless query and
applies the original native-controller hook before starting the app, so SDL sees
the selection from process startup. The query does not initialize SDL or open a
Qt window.

For scripted launches, the existing environment override remains supported:

```text
VIBEMIS_NATIVE_DECK_GYRO=1 %command%
```

The built-in Deck controls do not offer the same Steam Input disable option as
external controllers. This mode changes SDL's controller selection inside Vibemis
only: it exposes the native Deck controller and excludes Steam's virtual copy,
which has no motion sensors. Other programs and global Steam Input settings are
unchanged. Direct mode bypasses Steam Input custom button layouts for Vibemis.
Turn the toggle off and restart Vibemis to restore the existing behavior.
An explicit `VIBEMIS_NATIVE_DECK_GYRO=1` or `=0` launch override takes precedence
over the saved preference; remove it to let the in-app toggle control the mode.
Native mode activates only when the native controller's hidraw endpoint is
readable and writable.

The AppImage bundles pinned SDL 2.32.10 instead of the Ubuntu 22.04 build host's
SDL 2.0.20. A small carried SDL patch enables the Deck's raw gyro and accelerometer
reports when the application activates sensors. Some controller states leave the
IMU off, even though SDL reports both sensors as available. This writes only the
volatile IMU-mode setting; it does not install a driver, change firmware or
calibration, or start another input mapper. The patch can be dropped once the SDL
version pin includes equivalent upstream behavior.

## Checks

```sh
mkdir -p build-native-deck-tests
cd build-native-deck-tests
qmake6 ../tests/native-deck/native-deck.pro
make -j2
./tst_nativedeckcontroller
cd ..
bash tests/native-deck/test-controller-selection.sh
bash tests/native-deck/test-prelaunch-settings.sh build-native-deck-tests/tst_nativedeckcontroller
```

The AppImage CI additionally checks the deployed SDL runtime version to catch a
fallback to the older system library. Local AppImage builds use the same pinned
SDL build script and pre-launch hook as CI. The native Deck tests cover persisted
opt-in, hardware detection, filter preservation, launch overrides, and selection
reaching a new process before `main()`. AppImage CI also verifies the packaged
headless query against both disabled and enabled saved settings.

On a Deck, launch through Steam Game Mode, connect, and verify Vibemis identifies
`Steam Deck` with `gyro=yes accel=yes`. Actual sensor events should contain varying
gyro values and acceleration consistent with gravity, rather than all zeros.
Use a host that supports PlayStation motion emulation and a game with native
motion input enabled. No gyro-to-stick or gyro-to-mouse mapping is required.
