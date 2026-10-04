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

#include <QJsonObject>
#include <QList>
#include <QString>

// A game profile found in another launcher's data directory
struct DetectedProfile {
    /// Human-readable launcher name, e.g. "CurseForge"
    QString launcherName;
    /// Profile name
    QString name;
    /// Minecraft version string, may be empty if undetectable
    QString mcVersion;
    /// Loader component uid (e.g. "net.minecraftforge"), empty for vanilla/unknown
    QString loaderUid;
    /// Loader version string, may be empty
    QString loaderVersion;
    /// Directory holding the game content (mods, saves, config, etc.)
    QString contentDir;
};

namespace LauncherMigration {

/// Maps a loader name used by other launchers to a meta component uid.
/// Returns an empty string for vanilla or unknown loaders.
QString componentUidForLoader(const QString& loaderName);

/// Scan the standard data locations of other Minecraft launchers and
/// return every detected profile.
QList<DetectedProfile> detectProfiles();

// Exposed for unit tests

/// Guess loader info from a vanilla launcher version id, e.g.
/// "fabric-loader-0.15.0-1.20.1" or "1.20.1-forge-47.2.0"
void parseVersionId(const QString& versionId, QString& mcVersion, QString& loaderUid, QString& loaderVersion);

/// Read component versions of an mmc-pack.json object into
/// (mcVersion, loaderUid, loaderVersion)
void parseMmcPack(const QJsonObject& pack, QString& mcVersion, QString& loaderUid, QString& loaderVersion);

}  // namespace LauncherMigration
