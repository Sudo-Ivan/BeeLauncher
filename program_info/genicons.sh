#!/bin/bash

LAUNCHER_APPID="org.beelauncher.BeeLauncher"
LAUNCHER_APP_BINARY_NAME="beelauncher"
MASTER_PNG="${LAUNCHER_APPID}.source.png"

raster2png() {
    input_file="$1"
    output_file="$2"
    width="$3"
    height="$4"

    magick "$input_file" -resize "${width}x${height}" -strip PNG32:"$output_file"
}

svg2png() {
    input_file="$1"
    output_file="$2"
    width="$3"
    height="$4"

    inkscape -w "$width" -h "$height" -o "$output_file" "$input_file"
}

if [[ -f "$MASTER_PNG" ]] && command -v magick >/dev/null; then
    d=$(mktemp -d)

    raster2png "$MASTER_PNG" "$d/${LAUNCHER_APP_BINARY_NAME}_16.png" 16 16
    raster2png "$MASTER_PNG" "$d/${LAUNCHER_APP_BINARY_NAME}_24.png" 24 24
    raster2png "$MASTER_PNG" "$d/${LAUNCHER_APP_BINARY_NAME}_32.png" 32 32
    raster2png "$MASTER_PNG" "$d/${LAUNCHER_APP_BINARY_NAME}_48.png" 48 48
    raster2png "$MASTER_PNG" "$d/${LAUNCHER_APP_BINARY_NAME}_64.png" 64 64
    raster2png "$MASTER_PNG" "$d/${LAUNCHER_APP_BINARY_NAME}_128.png" 128 128
    raster2png "$MASTER_PNG" "$d/${LAUNCHER_APP_BINARY_NAME}_256.png" 256 256

    if command -v oxipng >/dev/null; then
        oxipng --opt max --strip all --alpha --interlace 0 "$d/${LAUNCHER_APP_BINARY_NAME}_"*".png"
    fi

    magick \
        "$d/${LAUNCHER_APP_BINARY_NAME}_256.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_128.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_64.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_48.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_32.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_24.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_16.png" \
        "${LAUNCHER_APP_BINARY_NAME}.ico"
    cp -v "$d/${LAUNCHER_APP_BINARY_NAME}_256.png" "${LAUNCHER_APPID}_256.png"
elif command -v inkscape >/dev/null && command -v icotool >/dev/null && command -v oxipng >/dev/null; then
    d=$(mktemp -d)

    svg2png ${LAUNCHER_APPID}.svg "$d/${LAUNCHER_APP_BINARY_NAME}_16.png" 16 16
    svg2png ${LAUNCHER_APPID}.svg "$d/${LAUNCHER_APP_BINARY_NAME}_24.png" 24 24
    svg2png ${LAUNCHER_APPID}.svg "$d/${LAUNCHER_APP_BINARY_NAME}_32.png" 32 32
    svg2png ${LAUNCHER_APPID}.svg "$d/${LAUNCHER_APP_BINARY_NAME}_48.png" 48 48
    svg2png ${LAUNCHER_APPID}.svg "$d/${LAUNCHER_APP_BINARY_NAME}_64.png" 64 64
    svg2png ${LAUNCHER_APPID}.svg "$d/${LAUNCHER_APP_BINARY_NAME}_128.png" 128 128
    svg2png ${LAUNCHER_APPID}.svg "$d/${LAUNCHER_APP_BINARY_NAME}_256.png" 256 256

    oxipng --opt max --strip all --alpha --interlace 0 "$d/${LAUNCHER_APP_BINARY_NAME}_"*".png"

    rm -f ${LAUNCHER_APP_BINARY_NAME}.ico
    icotool -o ${LAUNCHER_APP_BINARY_NAME}.ico -c \
        "$d/${LAUNCHER_APP_BINARY_NAME}_256.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_128.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_64.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_48.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_32.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_24.png" \
        "$d/${LAUNCHER_APP_BINARY_NAME}_16.png"
else
    echo "ERROR: Windows icons were NOT generated!" >&2
    echo "ERROR: requires magick plus ${MASTER_PNG}, or inkscape, icotool and oxipng"
fi

if command -v iconutil >/dev/null && command -v magick >/dev/null && [[ -f "$MASTER_PNG" ]]; then
    d=$(mktemp -d)
    d="$d/${LAUNCHER_APP_BINARY_NAME}.iconset"
    mkdir -p "$d"

    raster2png "$MASTER_PNG" "$d/icon_16x16.png" 16 16
    raster2png "$MASTER_PNG" "$d/icon_16x16@2x.png" 32 32
    raster2png "$MASTER_PNG" "$d/icon_32x32.png" 32 32
    raster2png "$MASTER_PNG" "$d/icon_32x32@2x.png" 64 64
    raster2png "$MASTER_PNG" "$d/icon_128x128.png" 128 128
    raster2png "$MASTER_PNG" "$d/icon_128x128@2x.png" 256 256
    raster2png "$MASTER_PNG" "$d/icon_256x256.png" 256 256
    raster2png "$MASTER_PNG" "$d/icon_256x256@2x.png" 512 512
    raster2png "$MASTER_PNG" "$d/icon_512x512.png" 512 512
    raster2png "$MASTER_PNG" "$d/icon_512x512@2x.png" 1024 1024

    if command -v oxipng >/dev/null; then
        oxipng --opt max --strip all --alpha --interlace 0 "$d/icon_"*".png"
    fi

    iconutil -c icns "$d"
    cp -v "$d/../${LAUNCHER_APP_BINARY_NAME}.icns" . 2>/dev/null || cp -v "${LAUNCHER_APP_BINARY_NAME}.icns" .
elif command -v inkscape >/dev/null && command -v iconutil >/dev/null && command -v oxipng >/dev/null; then
    d=$(mktemp -d)
    d="$d/${LAUNCHER_APP_BINARY_NAME}.iconset"
    mkdir -p "$d"

    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_16x16.png" 16 16
    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_16x16@2x.png" 32 32
    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_32x32.png" 32 32
    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_32x32@2x.png" 64 64
    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_128x128.png" 128 128
    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_128x128@2x.png" 256 256
    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_256x256.png" 256 256
    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_256x256@2x.png" 512 512
    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_512x512.png" 512 512
    svg2png ${LAUNCHER_APPID}.bigsur.svg "$d/icon_512x512@2x.png" 1024 1024

    oxipng --opt max --strip all --alpha --interlace 0 "$d/icon_"*".png"

    iconutil -c icns "$d"
    cp -v "$d/${LAUNCHER_APP_BINARY_NAME}.icns" .
else
    echo "NOTE: macOS icons were not regenerated (needs iconutil on macOS)" >&2
fi

cp -v ${LAUNCHER_APPID}.svg "../launcher/resources/multimc/scalable/launcher.svg"
