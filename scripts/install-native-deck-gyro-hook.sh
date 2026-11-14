#!/usr/bin/env bash
set -euo pipefail
source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
appdir=${1:?Usage: install-native-deck-gyro-hook.sh APPDIR}
mkdir -p "$appdir/apprun-hooks"
install -m 755 "$source_root/scripts/native-steam-deck-gyro.sh" \
    "$appdir/apprun-hooks/00-native-steam-deck-gyro.sh"
