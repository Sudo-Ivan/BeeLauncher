#include "ShellSettings.h"

#include <QDir>
#include <QFileInfo>
#include <QVariantMap>
#include <QtGlobal>

#include "../../../Application.h"
#include "../../../DesktopServices.h"
#include "../../../settings/Setting.h"
#include "../../../settings/SettingsObject.h"
#include "../../themes/ITheme.h"
#include "../../themes/IconTheme.h"
#include "../../themes/ThemeManager.h"
#include "BuildConfig.h"

ShellSettings::ShellSettings(QObject* parent) : QObject(parent) {}

void ShellSettings::attachBackend()
{
    m_backend = true;
    auto* s = APPLICATION->settings();
    connect(s, &SettingsObject::SettingChanged, this,
            [this](const Setting& setting, QVariant) { emit changed(setting.id()); });
    connect(s, &SettingsObject::settingReset, this, [this](const Setting& setting) { emit changed(setting.id()); });
}

SettingsObject* ShellSettings::store() const
{
    return m_backend ? APPLICATION->settings() : nullptr;
}

QVariant ShellSettings::value(const QString& key) const
{
    auto* s = store();
    if (!s || !s->getOrRegisterSetting(key, QVariant()))
        return {};
    return s->get(key);
}

bool ShellSettings::setValue(const QString& key, const QVariant& v)
{
    auto* s = store();
    if (!s)
        return false;
    if (!s->set(key, v))
        return false;
    emit changed(key);
    return true;
}

void ShellSettings::resetValue(const QString& key)
{
    auto* s = store();
    if (!s)
        return;
    s->reset(key);
    emit changed(key);
}

QString ShellSettings::dataRoot() const
{
    return m_backend ? APPLICATION->dataRoot() : QDir::currentPath();
}

bool ShellSettings::openSettingPath(const QString& key)
{
    auto* s = store();
    if (!s || !s->getOrRegisterSetting(key, QVariant()))
        return false;
    const QString path = s->get(key).toString();
    if (path.isEmpty())
        return false;
    const QFileInfo info(path);
    const QString abs = info.isAbsolute() ? info.absoluteFilePath() : QDir(APPLICATION->dataRoot()).absoluteFilePath(path);
    return DesktopServices::openPath(abs, true);
}

bool ShellSettings::openPath(const QString& absolutePath)
{
    if (absolutePath.isEmpty())
        return false;
    return DesktopServices::openPath(absolutePath, true);
}

static QVariantMap themeEntry(const QString& id, const QString& name, const QString& tooltip = QString())
{
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("name")] = name;
    if (!tooltip.isEmpty())
        m[QStringLiteral("tooltip")] = tooltip;
    return m;
}

QVariantList ShellSettings::iconThemes() const
{
    QVariantList out;
    if (!m_backend) {
        // Static fallback so the page still renders in previews and tests.
        out << themeEntry(QStringLiteral("fluent_dark"), tr("Fluent Dark"))
            << themeEntry(QStringLiteral("fluent"), tr("Fluent"))
            << themeEntry(QStringLiteral("pe_colored"), tr("Pocket Edition Colored"))
            << themeEntry(QStringLiteral("pe_dark"), tr("Pocket Edition Dark"))
            << themeEntry(QStringLiteral("pe_light"), tr("Pocket Edition Light"))
            << themeEntry(QStringLiteral("breeze_dark"), tr("Breeze Dark"))
            << themeEntry(QStringLiteral("breeze_light"), tr("Breeze Light"))
            << themeEntry(QStringLiteral("flat"), tr("Flat"))
            << themeEntry(QStringLiteral("flat_white"), tr("Flat White"))
            << themeEntry(QStringLiteral("multimc"), tr("MultiMC"));
        return out;
    }
    auto* tm = APPLICATION->themeManager();
    if (!tm)
        return out;
    tm->refresh();
    const auto themes = tm->getValidIconThemes();
    for (const IconTheme* t : themes)
        out << themeEntry(t->id(), t->name());
    return out;
}

QVariantList ShellSettings::appThemes() const
{
    QVariantList out;
    if (!m_backend) {
        out << themeEntry(QStringLiteral("bee"), tr("Bee Dark"))
            << themeEntry(QStringLiteral("bee-light"), tr("Bee Light"))
            << themeEntry(QStringLiteral("dark"), tr("Dark"))
            << themeEntry(QStringLiteral("bright"), tr("Bright"))
            << themeEntry(QStringLiteral("gruvbox"), tr("Gruvbox"))
            << themeEntry(QStringLiteral("system"), tr("System"));
        return out;
    }
    auto* tm = APPLICATION->themeManager();
    if (!tm)
        return out;
    tm->refresh();
    const auto themes = tm->getValidApplicationThemes();
    for (ITheme* t : themes)
        out << themeEntry(t->id(), t->name(), t->tooltip());
    return out;
}

QString ShellSettings::appVersion() const
{
    return BuildConfig.printableVersionString();
}

QString ShellSettings::qtVersion() const
{
    return QString::fromLatin1(qVersion());
}

QString ShellSettings::gitRef() const
{
    if (!BuildConfig.GIT_COMMIT.isEmpty())
        return BuildConfig.GIT_TAG + QStringLiteral("+") + BuildConfig.GIT_COMMIT.left(8);
    return BuildConfig.GIT_TAG;
}

void ShellSettings::useClassicUi()
{
    auto* s = store();
    if (!s)
        return;
    s->set(QStringLiteral("UiMode"), QStringLiteral("classic"));
    emit changed(QStringLiteral("UiMode"));
}
