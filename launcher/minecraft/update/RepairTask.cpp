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

#include "RepairTask.h"

#include <QtConcurrent/QtConcurrent>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/AssetsUtils.h"
#include "minecraft/Library.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/HttpMetaCache.h"

bool RepairVerifyTask::fileMatchesSha1(const QString& path, const QString& expectedSha1)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
        return false;
    QCryptographicHash hash(QCryptographicHash::Sha1);
    char buffer[256 * 1024];
    qint64 read;
    while ((read = file.read(buffer, sizeof(buffer))) > 0) {
        if (m_cancel.loadAcquire())
            return true;  // bail out, pretend the file is fine
        hash.addData(QByteArrayView(buffer, read));
    }
    if (read < 0)
        return false;
    return hash.result().toHex() == expectedSha1.toLatin1();
}

void RepairVerifyTask::verifyLibraries()
{
    auto components = m_inst->getPackProfile();
    auto profile = components->getProfile();
    auto metacache = APPLICATION->metacache();
    auto runtimeContext = m_inst->runtimeContext();

    QList<QPair<QList<LibraryPtr>, QString>> pools;
    QList<LibraryPtr> sharedLibs;
    sharedLibs.append(profile->getLibraries());
    sharedLibs.append(profile->getNativeLibraries());
    sharedLibs.append(profile->getMavenFiles());
    for (const auto& agent : profile->getAgents()) {
        sharedLibs.append(agent.library);
    }
    sharedLibs.append(profile->getMainJar());
    pools.append({ sharedLibs, {} });
    pools.append({ profile->getJarMods(), m_inst->jarModsDir() });

    // files declared local to the instance live under the local libraries dir
    const QString localPath = m_inst->getLocalLibraryPath();

    for (const auto& pool : pools) {
        const QString& overridePath = pool.second.isEmpty() ? localPath : pool.second;
        for (const auto& lib : pool.first) {
            if (!lib || m_cancel.loadAcquire())
                continue;
            for (const auto& expected : lib->expectedFiles(runtimeContext)) {
                m_checked++;
                QString fullPath;
                MetaEntryPtr entry;
                if (expected.local) {
                    // local files are stored by filename only inside the instance
                    fullPath = FS::PathCombine(overridePath, QFileInfo(expected.storage).fileName());
                } else {
                    entry = metacache->resolveEntry("libraries", expected.storage);
                    fullPath = entry->getFullPath();
                }

                QFileInfo info(fullPath);
                if (!info.exists() || info.size() == 0) {
                    m_broken++;
                    if (entry)
                        entry->setStale(true);
                    else
                        qWarning() << "Repair: missing local file" << fullPath;
                    continue;
                }
                if (!expected.sha1.isEmpty() && !fileMatchesSha1(fullPath, expected.sha1)) {
                    m_broken++;
                    if (entry) {
                        entry->setStale(true);
                        qDebug() << "Repair: checksum mismatch" << fullPath;
                    } else {
                        qWarning() << "Repair: corrupted local file" << fullPath;
                    }
                }
            }
        }
    }
}

void RepairVerifyTask::verifyAssets()
{
    auto components = m_inst->getPackProfile();
    auto profile = components->getProfile();
    auto assets = profile->getMinecraftAssets();
    if (!assets || assets->id.isEmpty())
        return;

    auto metacache = APPLICATION->metacache();

    auto indexEntry = metacache->resolveEntry("asset_indexes", assets->id + ".json");
    QString indexPath = indexEntry->getFullPath();
    if (!assets->sha1.isEmpty() && !fileMatchesSha1(indexPath, assets->sha1)) {
        m_broken++;
        metacache->evictEntry(indexEntry);
        return;
    }

    AssetsIndex index;
    if (!AssetsUtils::loadAssetsIndexJson(assets->id, indexPath, index)) {
        // index unreadable, evict so it gets fetched again
        m_broken++;
        metacache->evictEntry(indexEntry);
        return;
    }

    QDir objectsDir = QDir("assets/objects");
    for (auto& object : index.objects) {
        if (m_cancel.loadAcquire())
            return;
        m_checked++;
        QString fullPath = objectsDir.absoluteFilePath(object.getRelPath());
        QFileInfo info(fullPath);
        bool bad = !info.exists() || info.size() != object.size;
        if (!bad && !object.hash.isEmpty())
            bad = !fileMatchesSha1(fullPath, object.hash);
        if (bad) {
            m_broken++;
            QFile::remove(fullPath);
            qDebug() << "Repair: removed bad asset object" << fullPath;
        }
    }
}

void RepairVerifyTask::doWork()
{
    verifyLibraries();
    verifyAssets();
}

void RepairVerifyTask::executeTask()
{
    auto components = m_inst->getPackProfile();
    if (!components || !components->getProfile()) {
        emitFailed(tr("Instance components are not available."));
        return;
    }

    setStatus(tr("Verifying instance files..."));
    m_future = QtConcurrent::run(QThreadPool::globalInstance(), [this] { doWork(); });
    connect(&m_watcher, &QFutureWatcher<void>::finished, this, [this] {
        if (m_cancel.loadAcquire())
            emitAborted();
        else
            emitSucceeded();
    });
    m_watcher.setFuture(m_future);
}

bool RepairVerifyTask::abort()
{
    if (!m_future.isRunning())
        return false;
    m_cancel.storeRelease(1);
    m_future.waitForFinished();
    return true;
}
