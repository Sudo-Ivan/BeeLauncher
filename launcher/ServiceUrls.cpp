// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Bee Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Bee Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ServiceUrls.h"

#include "Application.h"
#include "BuildConfig.h"
#include "settings/SettingsObject.h"

namespace ServiceUrls {

static QString withTrailingSlash(QString url)
{
    if (!url.endsWith('/'))
        url.append('/');
    return url;
}

static QString mvaRoot(const QString& mvaUrl)
{
    return withTrailingSlash(mvaUrl.isEmpty() ? BuildConfig.MVA_URL : mvaUrl);
}

QString metaBase(const QString& metaOverride, bool mvaEnabled, const QString& mvaUrl)
{
    if (!metaOverride.isEmpty())
        return metaOverride;
    if (mvaEnabled)
        return mvaRoot(mvaUrl) + QStringLiteral("meta/v1/");
    return BuildConfig.META_URL;
}

QString resourceBase(const QString& resourceOverride, bool mvaEnabled, const QString& mvaUrl)
{
    if (!resourceOverride.isEmpty())
        return resourceOverride;
    if (mvaEnabled)
        return mvaRoot(mvaUrl) + QStringLiteral("assets/objects/");
    return BuildConfig.DEFAULT_RESOURCE_BASE;
}

QString savesBase(const QString& savesUrl)
{
    return withTrailingSlash(savesUrl.isEmpty() ? BuildConfig.SAVES_URL : savesUrl);
}

QString metaBase()
{
    auto s = APPLICATION->settings();
    return metaBase(s->get("MetaURLOverride").toString(), s->get("MvaEnabled").toBool(), s->get("MvaURL").toString());
}

QString resourceBase()
{
    auto s = APPLICATION->settings();
    return resourceBase(s->get("ResourceURLOverride").toString(), s->get("MvaEnabled").toBool(), s->get("MvaURL").toString());
}

QString savesBase()
{
    return savesBase(APPLICATION->settings()->get("SavesURL").toString());
}

}  // namespace ServiceUrls
