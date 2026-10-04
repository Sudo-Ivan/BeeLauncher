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

#include "DetectedProfile.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>

#include "FileSystem.h"

namespace LauncherMigration {

QString componentUidForLoader(const QString& loaderName)
{
    QString l = loaderName.toLower();
    if (l == "forge")
        return "net.minecraftforge";
    if (l == "fabric" || l == "fabric-loader")
        return "net.fabricmc.fabric-loader";
    if (l == "quilt" || l == "quilt-loader")
        return "org.quiltmc.quilt-loader";
    if (l == "neoforge" || l == "neo-forge")
        return "net.neoforged";
    if (l == "liteloader")
        return "com.mumfrey.liteloader";
    return {};
}

namespace {

QString homePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

QString appDataPath()
{
    // ~/.local/share on Linux, %APPDATA% on Windows, ~/Library/Application Support on macOS
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
}

QJsonObject readJsonObject(const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

}  // namespace

void parseMmcPack(const QJsonObject& pack, QString& mcVersion, QString& loaderUid, QString& loaderVersion)
{
    const auto components = pack.value("components").toArray();
    for (const auto& c : components) {
        const auto obj = c.toObject();
        const QString uid = obj.value("uid").toString();
        const QString version = obj.value("version").toString();
        if (uid == "net.minecraft") {
            mcVersion = version;
        } else if (uid == "net.minecraftforge" || uid == "net.fabricmc.fabric-loader" || uid == "org.quiltmc.quilt-loader" ||
                   uid == "net.neoforged" || uid == "com.mumfrey.liteloader") {
            loaderUid = uid;
            loaderVersion = version;
        }
    }
}

// Guess loader info from a vanilla launcher version id, e.g.
// "fabric-loader-0.15.0-1.20.1" or "1.20.1-forge-47.2.0"
void parseVersionId(const QString& versionId, QString& mcVersion, QString& loaderUid, QString& loaderVersion)
{
    static const QRegularExpression fabricRe(QStringLiteral("^fabric-loader-([\\d.]+)-(\\S+)$"));
    static const QRegularExpression forgeRe(QStringLiteral("^(\\S+)-forge-([\\d.]+)$"));
    static const QRegularExpression neoForgeRe(QStringLiteral("^neoforge-([\\d.]+)$"));

    auto fabric = fabricRe.match(versionId);
    if (fabric.hasMatch()) {
        loaderUid = "net.fabricmc.fabric-loader";
        loaderVersion = fabric.captured(1);
        mcVersion = fabric.captured(2);
        return;
    }
    auto forge = forgeRe.match(versionId);
    if (forge.hasMatch()) {
        loaderUid = "net.minecraftforge";
        mcVersion = forge.captured(1);
        loaderVersion = forge.captured(2);
        return;
    }
    auto neoforge = neoForgeRe.match(versionId);
    if (neoforge.hasMatch()) {
        loaderUid = "net.neoforged";
        loaderVersion = neoforge.captured(1);
        return;
    }
    mcVersion = versionId;
}

namespace {

// Detect MultiMC-family instances: <root>/<name>/mmc-pack.json + minecraft/
void detectMmcFamily(const QString& launcherName, const QString& instancesRoot, QList<DetectedProfile>& out)
{
    QDir root(instancesRoot);
    if (!root.exists())
        return;
    for (const auto& dir : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString packPath = FS::PathCombine(dir.absoluteFilePath(), "mmc-pack.json");
        if (!QFileInfo::exists(packPath))
            continue;
        QJsonObject pack = readJsonObject(packPath);
        if (pack.isEmpty())
            continue;

        QString contentDir = FS::PathCombine(dir.absoluteFilePath(), "minecraft");
        if (!QFileInfo::exists(contentDir))
            contentDir = FS::PathCombine(dir.absoluteFilePath(), ".minecraft");

        DetectedProfile p;
        p.launcherName = launcherName;
        p.name = dir.fileName();
        parseMmcPack(pack, p.mcVersion, p.loaderUid, p.loaderVersion);
        p.contentDir = contentDir;
        out.append(p);
    }
}

// Detect vanilla launcher profiles in launcher_profiles.json
void detectVanilla(const QString& minecraftDir, QList<DetectedProfile>& out)
{
    QSet<QString> seen;
    // the MS Store build of the launcher uses a separate profiles file
    for (const auto& fileName : { QStringLiteral("launcher_profiles.json"),
                                  QStringLiteral("launcher_profiles_microsoft_store.json") }) {
        QJsonObject launcherProfiles = readJsonObject(FS::PathCombine(minecraftDir, fileName));
        const auto profiles = launcherProfiles.value("profiles").toObject();
        for (const auto& profile : profiles) {
            const auto obj = profile.toObject();
            DetectedProfile p;
            p.launcherName = QStringLiteral("Minecraft Launcher");
            p.name = obj.value("name").toString();
            if (p.name.isEmpty() || seen.contains(p.name))
                continue;
            QString versionId = obj.value("lastVersionId").toString();
            parseVersionId(versionId, p.mcVersion, p.loaderUid, p.loaderVersion);
            // skip aliases like "latest-release" that do not map to a real version
            if (p.mcVersion.isEmpty() || !p.mcVersion.at(0).isDigit())
                continue;
            seen.insert(p.name);
            p.contentDir = minecraftDir;
            out.append(p);
        }
    }
}

// Detect CurseForge App profiles: <root>/<name>/minecraftinstance.json
void detectCurseForge(const QString& instancesRoot, QList<DetectedProfile>& out)
{
    QDir root(instancesRoot);
    if (!root.exists())
        return;
    for (const auto& dir : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QJsonObject meta = readJsonObject(FS::PathCombine(dir.absoluteFilePath(), "minecraftinstance.json"));
        if (meta.isEmpty())
            continue;

        DetectedProfile p;
        p.launcherName = QStringLiteral("CurseForge");
        p.name = dir.fileName();
        p.mcVersion = meta.value("gameVersion").toString();

        // "baseModLoader": { "name": "forge-47.2.0", ... }
        const auto loaderName = meta.value("baseModLoader").toObject().value("name").toString();
        static const QRegularExpression loaderRe(QStringLiteral("^(forge|fabric|quilt|neoforge|neo-forge|liteloader)-(.+)$"),
                                                 QRegularExpression::CaseInsensitiveOption);
        auto m = loaderRe.match(loaderName);
        if (m.hasMatch()) {
            p.loaderUid = componentUidForLoader(m.captured(1));
            p.loaderVersion = m.captured(2);
        }
        p.contentDir = dir.absoluteFilePath();
        out.append(p);
    }
}

// Detect Modrinth App profiles: <root>/<name>/profile.json
void detectModrinth(const QString& profilesRoot, QList<DetectedProfile>& out)
{
    QDir root(profilesRoot);
    if (!root.exists())
        return;
    for (const auto& dir : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QJsonObject meta = readJsonObject(FS::PathCombine(dir.absoluteFilePath(), "profile.json"));
        if (meta.isEmpty())
            continue;

        DetectedProfile p;
        p.launcherName = QStringLiteral("Modrinth App");
        p.name = meta.value("name").toString();
        if (p.name.isEmpty())
            p.name = dir.fileName();
        p.mcVersion = meta.value("game_version").toString();
        if (p.mcVersion.isEmpty())
            p.mcVersion = meta.value("mcVersion").toString();
        QString loader = meta.value("loader").toString();
        p.loaderUid = componentUidForLoader(loader);
        p.loaderVersion = meta.value("loader_version").toString();
        p.contentDir = dir.absoluteFilePath();
        out.append(p);
    }
}

// Detect GDLauncher instances: <root>/<name>/config.json
void detectGDLauncher(const QString& instancesRoot, QList<DetectedProfile>& out)
{
    QDir root(instancesRoot);
    if (!root.exists())
        return;
    for (const auto& dir : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QJsonObject meta = readJsonObject(FS::PathCombine(dir.absoluteFilePath(), "config.json"));
        if (meta.isEmpty())
            continue;

        DetectedProfile p;
        p.launcherName = QStringLiteral("GDLauncher");
        p.name = dir.fileName();
        const auto loader = meta.value("loader").toObject();
        p.mcVersion = loader.value("mcVersion").toString();
        if (p.mcVersion.isEmpty())
            p.mcVersion = meta.value("gameVersion").toString();
        p.loaderUid = componentUidForLoader(loader.value("loaderType").toString());
        p.loaderVersion = loader.value("loaderVersion").toString();
        p.contentDir = dir.absoluteFilePath();
        out.append(p);
    }
}

}  // namespace

QList<DetectedProfile> detectProfiles()
{
    QList<DetectedProfile> out;

    const QString home = homePath();
    const QString appData = appDataPath();

    // MultiMC-family instance roots
    const QList<QPair<QString, QString>> mmcRoots = {
        { QStringLiteral("MultiMC"), FS::PathCombine(home, ".local/share/multimc/instances") },
        { QStringLiteral("MultiMC"), FS::PathCombine(appData, "MultiMC/instances") },
        { QStringLiteral("Prism Launcher"), FS::PathCombine(appData, "PrismLauncher/instances") },
        { QStringLiteral("Prism Launcher"), FS::PathCombine(home, ".local/share/PrismLauncher/instances") },
        { QStringLiteral("Prism Launcher"),
          FS::PathCombine(home, ".var/app/org.prismlauncher.PrismLauncher/data/PrismLauncher/instances") },
        { QStringLiteral("PolyMC"), FS::PathCombine(appData, "PolyMC/instances") },
        { QStringLiteral("PolyMC"), FS::PathCombine(home, ".local/share/PolyMC/instances") },
        { QStringLiteral("Fjord Launcher"), FS::PathCombine(appData, "FjordLauncher/instances") },
        { QStringLiteral("Fjord Launcher"), FS::PathCombine(home, ".local/share/FjordLauncher/instances") },
        { QStringLiteral("Freesm Launcher"), FS::PathCombine(appData, "FreesmLauncher/instances") },
        { QStringLiteral("Freesm Launcher"), FS::PathCombine(home, ".local/share/FreesmLauncher/instances") },
        { QStringLiteral("ATLauncher"), FS::PathCombine(appData, "ATLauncher/instances") },
        { QStringLiteral("ATLauncher"), FS::PathCombine(home, ".local/share/ATLauncher/instances") },
    };
    for (const auto& root : mmcRoots) {
        detectMmcFamily(root.first, root.second, out);
    }

    // Vanilla launcher
#if defined(Q_OS_WIN)
    detectVanilla(FS::PathCombine(appData, ".minecraft"), out);
#elif defined(Q_OS_MACOS)
    detectVanilla(FS::PathCombine(home, "Library/Application Support/minecraft"), out);
#else
    detectVanilla(FS::PathCombine(home, ".minecraft"), out);
#endif

    // CurseForge App
    detectCurseForge(FS::PathCombine(home, "curseforge/minecraft/Instances"), out);
    detectCurseForge(FS::PathCombine(home, "Documents/curseforge/minecraft/Instances"), out);
    detectCurseForge(FS::PathCombine(appData, "CurseForge/Minecraft/Instances"), out);

    // Modrinth App
    detectModrinth(FS::PathCombine(appData, "com.modrinth.theseus/profiles"), out);
    detectModrinth(FS::PathCombine(home, ".local/share/com.modrinth.theseus/profiles"), out);
    detectModrinth(FS::PathCombine(home, ".config/com.modrinth.theseus/profiles"), out);
    detectModrinth(FS::PathCombine(home, "Library/Application Support/com.modrinth.theseus/profiles"), out);

    // GDLauncher
    detectGDLauncher(FS::PathCombine(appData, "gdlauncher_next/instances"), out);
    detectGDLauncher(FS::PathCombine(home, ".local/share/gdlauncher_next/instances"), out);

    return out;
}

}  // namespace LauncherMigration
