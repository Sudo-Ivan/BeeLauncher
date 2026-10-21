#!/bin/bash
# Bundles the Qt Quick modules used by the shell into a sharun directory.
# Usage: bundle-qml.sh <sharun dir> <qml source dir> <app qml dir>
set -euo pipefail

dest="$1"
qml_src="$2"
app_qml="$3"

scanner="$(find "$(dirname "$qml_src")" -maxdepth 2 -type f -name qmlimportscanner | head -n 1)"
if [ -z "$scanner" ]; then
    echo "qmlimportscanner not found next to $qml_src" >&2
    exit 1
fi

platforms="$(find "$dest/shared" -type d -path '*/plugins/platforms' | head -n 1)"
if [ -z "$platforms" ]; then
    echo "No bundled Qt plugins found in $dest/shared" >&2
    exit 1
fi
qt_dir="$(dirname "$(dirname "$platforms")")"

modules="$("$scanner" -rootPath "$app_qml" -importPath "$qml_src" | jq -r '.[] | select(.type == "module" and .path != null) | .path' | sort -u)"
if [ -z "$modules" ]; then
    echo "qmlimportscanner found no QML modules" >&2
    exit 1
fi

plugins=()
while IFS= read -r module; do
    rel="${module#"$qml_src"/}"
    mkdir -p "$qt_dir/qml/$rel"
    cp -a "$module"/. "$qt_dir/qml/$rel"/
    while IFS= read -r so; do
        plugins+=("$so")
    done < <(find "$module" -maxdepth 1 -name '*.so')
done <<< "$modules"

if [ "${#plugins[@]}" -gt 0 ]; then
    sharun lib4bin --hard-links --dst-dir "$dest" "${plugins[@]}"
fi

for required in QtQuick/qmldir QtQuick/Controls/Basic/qmldir QtQuick/Layouts/qmldir QtQml/qmldir; do
    if [ ! -f "$qt_dir/qml/$required" ]; then
        echo "Missing $required in the bundled QML modules" >&2
        exit 1
    fi
done

echo "Bundled $(echo "$modules" | wc -l) QML modules into $qt_dir/qml"
