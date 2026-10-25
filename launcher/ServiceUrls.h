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

#pragma once

#include <QString>

namespace ServiceUrls {

/// Base URL for launcher metadata. An explicit override always wins,
/// otherwise the MVA mirror is used when enabled.
QString metaBase(const QString& metaOverride, bool mvaEnabled, const QString& mvaUrl);

/// Base URL for game asset objects.
QString resourceBase(const QString& resourceOverride, bool mvaEnabled, const QString& mvaUrl);

/// saves server base URL, normalized with a trailing slash.
QString savesBase(const QString& savesUrl);

QString metaBase();
QString resourceBase();
QString savesBase();

}  // namespace ServiceUrls
