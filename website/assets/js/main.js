const RELEASES_URL = "https://github.com/Sudo-Ivan/BeeLauncher/releases/latest"
const API_URL = "https://api.github.com/repos/Sudo-Ivan/BeeLauncher/releases/latest"

const ASSET_PATTERNS = {
  "win-msvc-setup": /^BeeLauncher-Windows-MSVC-Setup-.*\.exe$/,
  "win-msvc-portable": /^BeeLauncher-Windows-MSVC-Portable-.*\.zip$/,
  "win-mingw-setup": /^BeeLauncher-Windows-MinGW-w64-Setup-.*\.exe$/,
  "win-mingw-portable": /^BeeLauncher-Windows-MinGW-w64-Portable-.*\.zip$/,
  "linux-appimage": /^BeeLauncher-Linux-x86_64\.AppImage$/,
  "linux-portable": /^BeeLauncher-Linux-Qt6-Portable-.*\.tar\.gz$/,
  "mac-dmg": /^BeeLauncher-macOS-.*\.dmg$/,
  "mac-zip": /^BeeLauncher-macOS-.*\.zip$/,
}

function detectOS() {
  const ua = navigator.userAgent || ""
  if (/Windows/i.test(ua)) return "windows"
  if (/Mac OS X|macOS/i.test(ua)) return "mac"
  if (/Linux|X11/i.test(ua)) return "linux"
  return null
}

function heroAssetFor(os) {
  if (os === "windows") return "win-msvc-setup"
  if (os === "mac") return "mac-dmg"
  if (os === "linux") return "linux-appimage"
  return null
}

async function loadRelease() {
  try {
    const res = await fetch(API_URL)
    if (!res.ok) return
    const data = await res.json()
    const assets = Array.isArray(data.assets) ? data.assets : []
    const links = {}
    for (const [key, re] of Object.entries(ASSET_PATTERNS)) {
      const hit = assets.find((a) => re.test(a.name))
      if (hit) links[key] = hit.browser_download_url
    }

    document.querySelectorAll("[data-asset]").forEach((el) => {
      const url = links[el.getAttribute("data-asset")]
      if (url) el.setAttribute("href", url)
    })

    const hero = document.getElementById("hero-download")
    const heroKey = heroAssetFor(detectOS())
    if (hero && heroKey && links[heroKey]) hero.setAttribute("href", links[heroKey])

    const line = document.getElementById("version-line")
    if (line && data.tag_name) {
      line.textContent = "Latest release: " + data.tag_name
    }
  } catch (e) {
    // fall back to the releases page link already in the markup
  }
}

function setupLightbox() {
  const dialog = document.getElementById("lightbox")
  if (!dialog || typeof dialog.showModal !== "function") return
  const img = document.getElementById("lightbox-img")
  const caption = document.getElementById("lightbox-caption")
  const closeBtn = document.getElementById("lightbox-close")

  document.querySelectorAll("[data-shot]").forEach((el) => {
    el.addEventListener("click", (ev) => {
      ev.preventDefault()
      img.setAttribute("src", el.getAttribute("data-shot"))
      img.setAttribute("alt", el.getAttribute("data-caption") || "")
      caption.textContent = el.getAttribute("data-caption") || ""
      dialog.showModal()
    })
  })

  closeBtn.addEventListener("click", () => dialog.close())
  dialog.addEventListener("click", (ev) => {
    if (ev.target === dialog) dialog.close()
  })
}

loadRelease()
setupLightbox()
