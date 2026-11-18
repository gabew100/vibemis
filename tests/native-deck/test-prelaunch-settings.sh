#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
query_binary=$(realpath "${1:?Usage: test-prelaunch-settings.sh PREFERENCE_QUERY_BINARY}")
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
appdir="$work/app with spaces"
mkdir -p "$appdir/usr/bin" "$work/config/Vibemis Project"
ln -s "$query_binary" "$appdir/usr/bin/vibemis"
cc -Wall -Wextra -Werror "$root/tests/native-deck/prelaunch-env-probe.c" -o "$work/probe"

run_case() {
    local expected=$1 available=$2
    NATIVE_DECK_EXPECT_NATIVE=$expected APPDIR=$appdir XDG_CONFIG_HOME="$work/config" \
        QT_QPA_PLATFORM=vibemis-no-platform-plugin \
        SDL_GAMECONTROLLER_IGNORE_DEVICES=0x28de/0x1205,0x054c/0x09cc \
        SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD=1 \
        SDL_JOYSTICK_HIDAPI=0 SDL_JOYSTICK_HIDAPI_STEAMDECK=0 \
        bash -c '
            source "$1"
            available=$3
            _vibemis_has_native_deck() { [ "$available" = 1 ]; }
            _vibemis_configure_native_deck
            exec "$2"
        ' _ "$root/scripts/native-steam-deck-gyro.sh" "$work/probe" "$available"
}

unset VIBEMIS_NATIVE_DECK_GYRO SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT DISPLAY WAYLAND_DISPLAY
printf '[General]\nnativedeckcontrols=false\n' > "$work/config/Vibemis Project/Vibemis.conf"
run_case 0 1
printf '[General]\nnativedeckcontrols=true\n' > "$work/config/Vibemis Project/Vibemis.conf"
run_case 1 1
run_case 0 0
export VIBEMIS_NATIVE_DECK_GYRO=0
run_case 0 1
printf '[General]\nnativedeckcontrols=false\n' > "$work/config/Vibemis Project/Vibemis.conf"
export VIBEMIS_NATIVE_DECK_GYRO=1
run_case 1 1
unset VIBEMIS_NATIVE_DECK_GYRO
appdir="$work/missing-app"
run_case 0 1
appdir="$work/failing-query"
mkdir -p "$appdir/usr/bin"
printf '#!/bin/sh\nprintf "1\\n"\nexit 1\n' > "$appdir/usr/bin/vibemis"
chmod +x "$appdir/usr/bin/vibemis"
run_case 0 1
export VIBEMIS_NATIVE_DECK_GYRO=1
run_case 1 1
echo 'PASS: saved opt-in and launch overrides reach the app before library initialization'
