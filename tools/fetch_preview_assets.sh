#!/bin/bash
# Fetch top Modrinth modpack icons + metadata for shell previews.
# Writes /tmp/bee-preview-assets/<slug>.png and <slug>.json. Not part of the app.
set -e
DIR=/tmp/bee-preview-assets
mkdir -p "$DIR"
UA="Sudo-Ivan/BeeLauncher preview tool"
curl -sf -A "$UA" 'https://api.modrinth.com/v2/search?limit=12&index=downloads&facets=%5B%5B%22project_type%3Amodpack%22%5D%5D' -o "$DIR/search.json"
python3 - "$DIR" <<'PY'
import json, sys, os, urllib.request
d = sys.argv[1]
hits = json.load(open(d + "/search.json"))["hits"]
for i, h in enumerate(hits):
    slug = h["slug"]
    meta = {
        "title": h["title"],
        "slug": slug,
        "game_version": h["versions"][-1] if h.get("versions") else "",
        "loader": h["display_categories"][0].capitalize() if h.get("display_categories") else "",
        "downloads": h.get("downloads", 0),
        "updated": h.get("date_modified", ""),
        "group": "Modded" if i % 3 else "Favorites",
    }
    json.dump(meta, open(os.path.join(d, slug + ".json"), "w"))
    icon = h.get("icon_url")
    if icon:
        try:
            urllib.request.urlretrieve(icon, os.path.join(d, slug + ".img"))
        except Exception as e:
            print("icon fail", slug, e)
print(len(hits), "packs")
PY
for f in "$DIR"/*.img; do
  [ -f "$f" ] && magick "$f" -resize "256x256> "${f%.img}.png" && rm "$f"
done
echo done
