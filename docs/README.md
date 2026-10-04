<div align="center">

<img width="160" height="160" align="center" src="./favicon.webp" alt="Favicon">

<h1>
<a style="color:#e8a33d" href="https://github.com/Sudo-Ivan/BeeLauncher">Bee Launcher</a>
</h1>

A FreesmLauncher fork that **removes offline account restrictions**, adds custom auth server support, and provides more customization

This fork is **not** endorsed by FreesmLauncher or Prism Launcher

Based on FreesmLauncher (Prism Launcher **11.1.1**)

<p align="center">
<strong>English</strong> | <a style="color:#e8a33d" href="./README_ru.md">Русский</a>
</p>

<div>

[![GitHub Repo stars](https://img.shields.io/github/stars/Sudo-Ivan/BeeLauncher?label=Stars&style=for-the-badge&color=%23e8a33d&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0idXRmLTgiPz4KPHN2ZyBoZWlnaHQ9IjI0IiB2aWV3Qm94PSIwIC05NjAgOTYwIDk2MCIgd2lkdGg9IjI0IiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciPgogIDxwYXRoIGQ9Im0zNTQtMjQ3IDEyNi03NiAxMjYgNzctMzMtMTQ0IDExMS05Ni0xNDYtMTMtNTgtMTM2LTU4IDEzNS0xNDYgMTMgMTExIDk3LTMzIDE0M1pNMjMzLTgwbDY1LTI4MUw4MC01NTBsMjg4LTI1IDExMi0yNjUgMTEyIDI2NSAyODggMjUtMjE4IDE4OSA2NSAyODEtMjQ3LTE0OUwyMzMtODBabTI0Ny0zNTBaIiBzdHlsZT0iZmlsbDogcmdiKDI0NSwgMTk0LCAyMzEpOyIvPgo8L3N2Zz4%3D)](https://github.com/Sudo-Ivan/BeeLauncher/stargazers)
![DRM Free Badge](https://img.shields.io/badge/drm-free-%23e8a33d?style=for-the-badge)

</div>

</div>

## Screenshots

<details>
  <summary>Show</summary>

  <div align="center">
    <div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px;">
      <img src="screenshots/beelauncher_home_screenshot.png" alt="Dark theme dashboard" width="512" />
      <img src="screenshots/beelauncher_home_screenshot_opacity.png" alt="Dashboard with opacity" width="512" />
      <img src="screenshots/beelauncher_settings_accounts_screenshot.png" alt="Accounts settings" width="512" />
      <img src="screenshots/beelauncher_instance_add_screenshot.png" alt="Add instance" width="512" />
      <img src="screenshots/beelauncher_instance_settings_screenshot.png" alt="Instance settings" width="512" />
      <img src="screenshots/beelauncher_settings_theme_screenshot.png" alt="Theme settings" width="512" />
    </div>
  </div>

</details>

## Features

- Offline mode doesn't require signing in with a Microsoft account anymore
- [Ely.by](https://ely.by/) can be used as an account auth option, providing a seamless integration with Minecraft. You will see your Minecraft skin anywhere without any mods or plugins
- Custom authentication server support
- Polished, minimalist dark and light themes based on a [Fluent-Dark](https://github.com/PrismLauncher/Themes/tree/main/themes/Fluent-Dark) theme with honey accent colors and [Microsoft Fluent](https://fluent2.microsoft.design/iconography) icons
- In-game screenshots copying to the buffer history without any mods support
- Animated snow effect for those who love... snow?
- Random username and instance icon selection with ultra-super-advanced and cryptographically secure, absolutely random number generator based on the [lavarand](https://www.youtube.com/watch?v=dQw4w9WgXcQ)
- FLOSS
- ...all the Prism Launcher's features

## Comparison


| Feature                                  | Bee  Launcher | Freesm Launcher | Shattered  Prism | HMCL | Fjord   | PollyMC       | PineconeMC      | UltimMC | Prism-Cracked | Prism Launcher |
|------------------------------------------|---------------|-----------------|------------------|------|---------|---------------|---------------|---------|---------------|----------------|
| Offline Mode without a Microsoft account | ✅             | ✅               | ✅                | ✅    | ❌       | ✅             | ✅             | ✅       | ✅             | ❌              |
| FTB packs                                | ✅             | ✅               | ✅                | ❌    | ✅       | ✅             | ✅             | ❌       | ✅             | ✅              |
| Ely.by support                           | ✅             | ✅               | 🟨¹              | 🟨¹  | 🟨¹     | 🟨¹           | ✅             | 🟨¹     | ❌             | ❌              |
| Authlib-injector support                 | ✅             | ✅               | ✅                | ✅    | ✅       | ✅             | ✅            | ❌²      | ❌²            | ❌²             |
| Screenshots saving to the buffer history | ✅             | ✅               | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Discord Rich Presence                    | ❌             | ✅               | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Built-in news feed                       | ❌             | ✅               | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Cat packs and anime content              | ❌             | ✅               | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Fork                                     | FreesmLauncher | PrismLauncher   | FjordLauncher    | ❌    | PollyMC | PrismLauncher | PrismLauncher | MultiMC | PrismLauncher | PolyMC         |

¹ doesn't use official Ely.by authlib patches

² you can still change a `javaagent` JVM argument to use your `authlib-injector` jar file as an auth server

Bee Launcher also disables the built-in updater and the upstream network integrations it had no replacement for, such as news, socials and update checks.

## Installation

### Stable Releases

Download Bee Launcher from our [official website](https://github.com/Sudo-Ivan/BeeLauncher) or the [GitHub Releases](https://github.com/Sudo-Ivan/BeeLauncher/releases) page. Packages are available for **Linux, Windows, and macOS**.

### Development builds

Please understand that these builds are not intended for most users. There may be bugs and other instabilities. You have been warned.

There are development builds available through:

* [GitHub Actions](https://github.com/Sudo-Ivan/BeeLauncher/actions) (includes builds from pull requests opened by contributors).
* [nightly.link](https://nightly.link/Sudo-Ivan/BeeLauncher/workflows/build/master) (this will always point only to the latest version of the `master` branch).

These builds contain debug information in the binaries, so their file sizes are relatively larger. Prebuilt Development builds are provided for **Linux, Windows, and macOS**.

## Community & Support

If you found a bug or want to suggest a feature, please open an issue in [GitHub Issues](https://github.com/Sudo-Ivan/BeeLauncher/issues). Pull requests and contributions (code, docs, translations) are welcome!

[![GitHub](https://img.shields.io/github/license/Sudo-Ivan/BeeLauncher?style=for-the-badge)](https://github.com/Sudo-Ivan/BeeLauncher/blob/master/LICENSE)
