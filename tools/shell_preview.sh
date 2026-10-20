#!/bin/bash
# Headless preview captures for the QML shell. Uses the Qt offscreen platform
# with OpenGL via QSG_RHI_BACKEND, so no window appears on the desktop.
# Usage: shell_preview.sh --page library --width 1280 --height 800 --output out.png [--theme light] [--select N] [--hover N] [--menu] [--empty 1] [--assets /tmp/bee-preview-assets]
set -e
BIN=$(find /tmp/bee-build -name BeeShellPreview -type f | head -1)
exec env QT_QPA_PLATFORM=offscreen QSG_RHI_BACKEND=opengl "$BIN" "$@"
