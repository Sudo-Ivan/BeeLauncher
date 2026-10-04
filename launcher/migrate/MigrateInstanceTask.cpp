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

#include "MigrateInstanceTask.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

namespace {

// Top-level entries inside a foreign game directory that must not be copied:
// launcher bookkeeping, downloaded dependencies and transient state.
const QSet<QString> s_deniedTopLevel = { "versions",
                                       "assets",
                                       "libraries",
                                       "natives",
                                       "logs",
                                       "crash-reports",
                                       "backups",
                                       ".index",
                                       ".fabric",
                                       "mmc-pack.json",
                                       "instance.cfg",
                                       "minecraftinstance.json",
                                       "profile.json",
                                       "config.json",
                                       "launcher_profiles.json",
                                       "launcher_settings.json",
                                       "manifest.json",
                                       "icon.png" };

}  // namespace

bool MigrateInstanceTask::copyRecursively(const QString& sourceDir, const QString& destDir)
{
    QDir source(sourceDir);
    QDirIterator it(sourceDir, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const auto info = it.fileInfo();
        const QString rel = source.relativeFilePath(info.absoluteFilePath());
        const QString destPath = FS::PathCombine(destDir, rel);

        if (info.isDir()) {
            if (!FS::ensureFolderPathExists(destPath))
                return false;
            continue;
        }
        if (!FS::ensureFilePathExists(destPath))
            return false;
        if (!QFile::copy(info.absoluteFilePath(), destPath))
            return false;
    }
    return true;
}

bool MigrateInstanceTask::copyContent(const QString& destinationRoot)
{
    QDir content(m_profile.contentDir);
    for (const auto& entry : content.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System)) {
        if (s_deniedTopLevel.contains(entry.fileName()))
            continue;
        const QString destPath = FS::PathCombine(destinationRoot, entry.fileName());
        if (entry.isDir()) {
            if (!copyRecursively(entry.absoluteFilePath(), destPath))
                return false;
        } else {
            if (!FS::ensureFilePathExists(destPath))
                return false;
            if (!QFile::copy(entry.absoluteFilePath(), destPath))
                return false;
        }
    }
    return true;
}

std::unique_ptr<MinecraftInstance> MigrateInstanceTask::createInstance()
{
    auto inst = std::make_unique<MinecraftInstance>(
        m_globalSettings, std::make_unique<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg")), m_stagingPath);
    SettingsObject::Lock lock(inst->settings());

    auto components = inst->getPackProfile();
    components->buildingFromScratch();
    if (!m_profile.mcVersion.isEmpty())
        components->setComponentVersion("net.minecraft", m_profile.mcVersion, true);
    if (!m_profile.loaderUid.isEmpty())
        components->setComponentVersion(m_profile.loaderUid, m_profile.loaderVersion);

    inst->setName(name());
    inst->setIconKey(m_instIcon);

    if (!m_profile.contentDir.isEmpty() && QFileInfo::exists(m_profile.contentDir)) {
        setStatus(tr("Copying files from %1...").arg(m_profile.launcherName));
        if (!copyContent(inst->gameRoot())) {
            setError(tr("Failed to copy files from %1").arg(m_profile.contentDir));
            return nullptr;
        }
    }

    return inst;
}
