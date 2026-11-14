#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
source "$root/scripts/native-steam-deck-gyro.sh"
fail() { echo "FAIL: $*" >&2; exit 1; }
assert_equal() { [ "$1" = "$2" ] || fail "$3: '$1' != '$2'"; }

_vibemis_has_native_deck() { return 0; }
export SDL_GAMECONTROLLER_IGNORE_DEVICES=0x28de/0x1205,0x054c/0x09cc
export SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD=1
unset VIBEMIS_NATIVE_DECK_GYRO
_vibemis_configure_native_deck
assert_equal "$SDL_GAMECONTROLLER_IGNORE_DEVICES" '0x28de/0x1205,0x054c/0x09cc' 'default preserves mappings'
assert_equal "$SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD" 1 'default preserves virtual gamepad'

export VIBEMIS_NATIVE_DECK_GYRO=1
_vibemis_has_native_deck() { return 1; }
_vibemis_configure_native_deck
assert_equal "$SDL_GAMECONTROLLER_IGNORE_DEVICES" '0x28de/0x1205,0x054c/0x09cc' 'no Deck preserves mappings'

_vibemis_has_native_deck() { return 0; }
export SDL_GAMECONTROLLER_IGNORE_DEVICES='0x28de/0x1205,0x054c/0x09cc,0x28DE/0x1205,0x28de/0x11ff,'
export SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT='0x28de/0x11ff'
_vibemis_configure_native_deck
assert_equal "$SDL_GAMECONTROLLER_IGNORE_DEVICES" '0x054c/0x09cc,0x28de/0x11ff' 'native exposed; duplicates removed; other exclusion preserved'
assert_equal "$SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT" '0x28de/0x1205' 'allowlist selects native controller'
assert_equal "$SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD" 0 'virtual copy disabled'
assert_equal "$SDL_JOYSTICK_HIDAPI_STEAMDECK" 1 'native driver enabled'

first=$SDL_GAMECONTROLLER_IGNORE_DEVICES
_vibemis_configure_native_deck
assert_equal "$SDL_GAMECONTROLLER_IGNORE_DEVICES" "$first" 'configuration is idempotent'
unset SDL_GAMECONTROLLER_IGNORE_DEVICES SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT
_vibemis_configure_native_deck
assert_equal "$SDL_GAMECONTROLLER_IGNORE_DEVICES" '0x28de/0x11ff' 'empty filter excludes only virtual copy'
[ -z "${SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT+x}" ] || fail 'absent allowlist was introduced'
echo 'PASS: native Deck selection is opt-in, hardware-gated, scoped, and idempotent'
