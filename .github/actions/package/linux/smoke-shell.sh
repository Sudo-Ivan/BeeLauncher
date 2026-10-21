#!/bin/bash
# Starts a packaged sharun directory headless and fails unless the Qt Quick shell renders its first frame.
# Usage: smoke-shell.sh <sharun dir>
set -euo pipefail

dir="$(realpath "$1")"
data="$(mktemp -d)"
log="$(mktemp)"
trap 'rm -rf "$data" "$log"' EXIT

mkdir -p "$data/instances"
cat > "$data/beelauncher.cfg" <<CFG
[General]
Language=en_US
IgnoreJavaWizard=true
AutomaticJavaDownload=true
AutomaticJavaSwitch=true
UserAskedAboutAutomaticJavaDownload=true
LastHostname=$(hostname)
ApplicationTheme=bee
IconTheme=fluent_dark
FlameKeyShouldBeFetchedOnStartup=false
CFG
python3 - "$data" <<'PY'
import json, sys, time, uuid
now = int(time.time())
account = {
    "type": "Offline",
    "active": True,
    "ygg": {"iat": now, "token": "0", "extra": {"userName": "CI", "clientToken": uuid.uuid4().hex}},
    "profile": {"id": uuid.uuid4().hex, "name": "CI", "skin": {"id": "", "url": "", "variant": ""}, "capes": []},
    "entitlement": {"ownsMinecraft": True, "canPlayMinecraft": True},
}
json.dump({"accounts": [account], "formatVersion": 3}, open(sys.argv[1] + "/accounts.json", "w"))
PY

session=()
if command -v dbus-run-session > /dev/null; then
    session=(dbus-run-session --)
fi

cd "$dir"
"${session[@]}" env -u QML2_IMPORT_PATH -u QML_IMPORT_PATH -u QT_PLUGIN_PATH -u QT_ROOT_DIR -u LD_LIBRARY_PATH -u Qt6_DIR \
    QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software BEE_STARTUP_PROFILE=1 BEE_SHELL_GRAB="$data/shell.png" \
    timeout 60 ./sharun beelauncher -d "$data" > "$log" 2>&1 || true

if grep -q "falling back to the classic interface" "$log"; then
    grep -E "Warning: qrc|not installed|falling back" "$log" | head -n 10 >&2
    echo "The packaged build could not load the Qt Quick shell" >&2
    exit 1
fi
if ! grep -q "first frame" "$log" || [ ! -s "$data/shell.png" ]; then
    tail -n 40 "$log" >&2
    echo "The packaged build did not render the Qt Quick shell" >&2
    exit 1
fi
echo "Qt Quick shell smoke test passed"
