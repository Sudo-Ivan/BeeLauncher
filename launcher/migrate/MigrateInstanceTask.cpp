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

#include <QtConcurrent/QtConcurrent>

#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QSet>
#include <QTimer>

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

bool MigrateInstanceTask::abort()
{
    m_cancel.storeRelease(1);
    return InstanceCreationTask::abort();
}

bool MigrateInstanceTask::copyRecursively(const QString& sourceDir, const QString& destDir)
{
    QDir source(sourceDir);
    QDirIterator it(sourceDir, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const auto info = it.fileInfo();
        if (m_cancel.loadAcquire())
            return false;
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
        m_copiedFiles.fetchAndAddRelease(1);
    }
    return true;
}

bool MigrateInstanceTask::copyContent(const QString& destinationRoot)
{
    QDir content(m_profile.contentDir);
    for (const auto& entry : content.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System)) {
        if (m_cancel.loadAcquire())
            return false;
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
        setStatus(tr("Copying game files from %1...").arg(m_profile.launcherName));
        m_copiedFiles.storeRelease(0);
        const QString gameRoot = inst->gameRoot();

        // run the copy on a worker thread and pump events so the dialog and
        // the abort button stay responsive
        QFutureWatcher<bool> watcher;
        QEventLoop loop;
        connect(&watcher, &QFutureWatcher<bool>::finished, &loop, &QEventLoop::quit);
        QTimer progressTimer;
        progressTimer.setInterval(200);
        connect(&progressTimer, &QTimer::timeout, this, [this] { setDetails(tr("%1 files copied").arg(m_copiedFiles.loadAcquire())); });
        progressTimer.start();
        watcher.setFuture(QtConcurrent::run(QThreadPool::globalInstance(), [this, gameRoot] { return copyContent(gameRoot); }));
        loop.exec();
        progressTimer.stop();

        if (!watcher.result()) {
            // remove the partially copied content so no half-migrated game data is left
            FS::deletePath(gameRoot);
            if (!m_cancel.loadAcquire())
                setError(tr("Failed to copy files from %1").arg(m_profile.contentDir));
            return nullptr;
        }
    }

    return inst;
}
