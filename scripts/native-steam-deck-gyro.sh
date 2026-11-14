#!/usr/bin/env bash
# Sourced by AppRun. Native Deck controls bypass Steam Input's custom layout,
# so leave the existing controller-selection behavior unchanged unless opted in.

_vibemis_has_native_deck() {
    local uevent device
    for uevent in /sys/class/hidraw/hidraw*/device/uevent; do
        device=${uevent%/device/uevent}
        device=/dev/${device##*/}
        if [ -r "$device" ] && [ -w "$device" ] && \
                grep -qi '^HID_ID=[^:]*:000028DE:00001205$' "$uevent" 2>/dev/null; then
            return 0
        fi
    done
    return 1
}

_vibemis_configure_native_deck() {
    [ "${VIBEMIS_NATIVE_DECK_GYRO:-0}" = 1 ] || return 0
    _vibemis_has_native_deck || return 0

    local id
    local -a existing=() filtered=()
    IFS=, read -r -a existing <<< "${SDL_GAMECONTROLLER_IGNORE_DEVICES:-}"
    for id in "${existing[@]}"; do
        case "${id,,}" in
            0x28de/0x1205|0x28de/0x11ff|'') ;;
            *) filtered+=("$id") ;;
        esac
    done
    # Hide the virtual copy so host motion requests target the native controller.
    filtered+=("0x28de/0x11ff")
    export SDL_GAMECONTROLLER_IGNORE_DEVICES="$(IFS=,; printf '%s' "${filtered[*]}")"
    if [ -n "${SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT:-}" ]; then
        export SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0x28de/0x1205
    fi
    export SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD=0
    export SDL_JOYSTICK_HIDAPI=1
    export SDL_JOYSTICK_HIDAPI_STEAMDECK=1
    echo '[motion] Native Steam Deck controller selected; Steam virtual copy excluded' >&2
    return 0
}

_vibemis_configure_native_deck
