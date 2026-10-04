<div align="center">

<img width="160" height="160" align="center" src="./favicon.webp" alt="Favicon">

<h1>
<a style="color:#e8a33d" href="https://github.com/Sudo-Ivan/BeeLauncher">Bee Launcher</a>
</h1>

Форк FreesmLauncher, который **позволяет играть с офлайн-аккаунтом без ограничений**, поддерживает кастомные сервера авторизации и расширяет кастомизацию

Этот форк **не** поддерживается FreesmLauncher или Prism Launcher'ом

Основан на FreesmLauncher (Prism Launcher **11.1.1**)

<p align="center">
<a style="color:#e8a33d" href="./README.md">English</a> | <strong>Русский</strong>
</p>

<div>

[![GitHub Repo stars](https://img.shields.io/github/stars/Sudo-Ivan/BeeLauncher?label=Stars&style=for-the-badge&color=%23e8a33d&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0idXRmLTgiPz4KPHN2ZyBoZWlnaHQ9IjI0IiB2aWV3Qm94PSIwIC05NjAgOTYwIDk2MCIgd2lkdGg9IjI0IiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciPgogIDxwYXRoIGQ9Im0zNTQtMjQ3IDEyNi03NiAxMjYgNzctMzMtMTQ0IDExMS05Ni0xNDYtMTMtNTgtMTM2LTU4IDEzNS0xNDYgMTMgMTExIDk3LTMzIDE0M1pNMjMzLTgwbDY1LTI4MUw4MC01NTBsMjg4LTI1IDExMi0yNjUgMTEyIDI2NSAyODggMjUtMjE4IDE4OSA2NSAyODEtMjQ3LTE0OUwyMzMtODBabTI0Ny0zNTBaIiBzdHlsZT0iZmlsbDogcmdiKDI0NSwgMTk0LCAyMzEpOyIvPgo8L3N2Zz4%3D)](https://github.com/Sudo-Ivan/BeeLauncher/stargazers)
![DRM Free Badge](https://img.shields.io/badge/drm-free-%23e8a33d?style=for-the-badge)

</div>

</div>

## Скриншоты

<details>
  <summary>Показать</summary>

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

## Возможности

- Офлайн-аккаунт больше не требует наличия лицензии
- Возможность входа через [Ely.by](https://ely.by/). Скины будут показываться не только на серверах с плагином Ely.by, но и в синглплеере/на серверах без плагина
- Поддержка кастомных серверов авторизации
- Темная и светлая темы на основе [Fluent-Dark](https://github.com/PrismLauncher/Themes/tree/main/themes/Fluent-Dark) с медовыми акцентными цветами и иконками [Microsoft Fluent](https://fluent2.microsoft.design/iconography)
- Поддержка автоматического копирования скриншотов из игры в буфер обмена без модов
- Анимированный эффект снегопада
- Выбор случайных никнеймов и иконок для сборок
- FLOSS
- ...все остальные фичи Prism Launcher'а

## Сравнение


| Feature                                           | Bee  Launcher | Freesm Launcher | Shattered  Prism | HMCL | Fjord   | PollyMC       | PineconeMC    | UltimMC | Prism-Cracked | Prism Launcher |
|---------------------------------------------------|---------------|-----------------|------------------|------|---------|---------------|---------------|---------|---------------|----------------|
| Офлайн-игра без аккаунта Microsoft                | ✅             | ✅               | ✅                | ✅    | ❌       | ✅             | ✅             | ✅       | ✅             | ❌              |
| FTB сборки                                        | ✅             | ✅               | ✅                | ❌    | ✅       | ✅             | ✅             | ❌       | ✅             | ✅              |
| Поддержка Ely.by                                  | ✅             | ✅               | 🟨¹              | 🟨¹  | 🟨¹     | 🟨¹           | ✅             | 🟨¹     | ❌             | ❌              |
| Поддержка Authlib-injector                        | ✅             | ✅               | ✅                | ✅    | ✅       | ✅             | ✅             | ❌²      | ❌²            | ❌²             |
| Копирование скриншотов из игры в буфер обмена     | ✅             | ✅               | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Discord Rich Presence                             | ❌             | ✅               | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Встроенная лента новостей                         | ❌             | ✅               | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Наборы котов и аниме-контент                      | ❌             | ✅               | ❌                | ❌    | ❌       | ❌             | ❌             | ❌       | ❌             | ❌              |
| Форк                                              | FreesmLauncher | PrismLauncher   | FjordLauncher    | ❌    | PollyMC | PrismLauncher | PrismLauncher | MultiMC | PrismLauncher | PolyMC         |

¹ не использует официальные Ely.by authlib патчи

² можно изменить JVM аргумент `javaagent` так, чтобы он использовал Ваш файл `authlib-injector` как сервер авторизации

Bee Launcher также отключает встроенный автообновлятель и сетевые интеграции upstream, для которых нет замены: новости, социальные ссылки и проверки обновлений.

## Установка

### Стабильные версии

Скачайте Bee Launcher с нашего [официального сайта](https://github.com/Sudo-Ivan/BeeLauncher) или через страницу [GitHub Releases](https://github.com/Sudo-Ivan/BeeLauncher/releases). Лаунчер доступен на **Linux, Windows и macOS**.


### Нестабильные сборки

Имейте в виду, что эти сборки могут содержать ошибки и быть нестабильными. Мы не рекомендуем использовать их в большинстве случаев.

Доступные нестабильные сборки могут быть получены через:

* [GitHub Actions](https://github.com/Sudo-Ivan/BeeLauncher/actions) (также включает сборки из pull-реквестов контрибьюторов).
* [nightly.link](https://nightly.link/Sudo-Ivan/BeeLauncher/workflows/build/master) (ссылка всегда будет указывать на последнюю версию ветки `master`).

Эти сборки содержат отладочную информацию, поэтому их размер будет относительно больше. Уже готовые нестабильные сборки доступны на **Linux, Windows и macOS**.

## Сообщество и поддержка

Если Вы нашли баг или хотите сделать какое-либо предложение, пожалуйста, откройте issue в [GitHub Issues](https://github.com/Sudo-Ivan/BeeLauncher/issues). Pull-реквесты и любой вклад (code, docs, translations) приветствуются!

[![GitHub](https://img.shields.io/github/license/Sudo-Ivan/BeeLauncher?style=for-the-badge)](https://github.com/Sudo-Ivan/BeeLauncher/blob/master/LICENSE)
