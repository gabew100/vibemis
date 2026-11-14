# Native Steam Deck motion forwarding (AppImage)

For native motion input in games and emulators, enable **Forward motion controls
(gyro)** in Vibemis. In the Steam shortcut's launch options, opt in to direct Deck
controller access:

```text
VIBEMIS_NATIVE_DECK_GYRO=1 %command%
```

The built-in Deck controls do not offer the same Steam Input disable option as
external controllers. This mode changes SDL's controller selection inside Vibemis
only: it exposes the native Deck controller and excludes Steam's virtual copy,
which has no motion sensors. Other programs and global Steam Input settings are
unchanged. Direct mode bypasses Steam Input custom button layouts for Vibemis;
leave the option unset to retain the existing behavior. It activates only when
the native controller's hidraw endpoint is readable and writable.

The AppImage bundles pinned SDL 2.32.10 instead of the Ubuntu 22.04 build host's
SDL 2.0.20. A small carried SDL patch enables the Deck's raw gyro and accelerometer
reports when the application activates sensors. Some controller states leave the
IMU off, even though SDL reports both sensors as available. This writes only the
volatile IMU-mode setting; it does not install a driver, change firmware or
calibration, or start another input mapper. The patch can be dropped once the SDL
version pin includes equivalent upstream behavior.

## Checks

```sh
bash tests/native-deck/test-controller-selection.sh
```

The AppImage CI additionally checks the deployed SDL runtime version to catch a
fallback to the older system library. Local AppImage builds use the same pinned
SDL build script and controller-selection hook as CI.

On a Deck, launch through Steam Game Mode, connect, and verify Vibemis identifies
`Steam Deck` with `gyro=yes accel=yes`. Actual sensor events should contain varying
gyro values and acceleration consistent with gravity, rather than all zeros.
Use a host that supports PlayStation motion emulation and a game with native
motion input enabled. No gyro-to-stick or gyro-to-mouse mapping is required.
