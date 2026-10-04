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

#include "InstanceBackup.h"

#include <QtConcurrent/QtConcurrent>

#include <QDateTime>
#include <QRegularExpression>

#include "BaseInstance.h"
#include "FileSystem.h"
#include "MMCZip.h"
#include "archive/ArchiveWriter.h"

namespace InstanceBackup {

QString backupsDirPath(BaseInstance* instance)
{
    return FS::PathCombine(instance->instanceRoot(), "backups");
}

QString generateBackupFileName(const QString& instanceName, const QString& backupsDir)
{
    // strip characters that are not allowed in filenames on any platform
    QString name = instanceName;
    name.remove(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")));
    name = name.trimmed();
    if (name.isEmpty())
        name = QStringLiteral("instance");
    QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString candidate = QString("%1-%2.zip").arg(name, stamp);
    int suffix = 1;
    while (QFileInfo::exists(FS::PathCombine(backupsDir, candidate))) {
        candidate = QString("%1-%2-%3.zip").arg(name, stamp).arg(suffix++);
    }
    return candidate;
}

QString generateBackupFileName(BaseInstance* instance)
{
    return generateBackupFileName(instance->name(), backupsDirPath(instance));
}

QList<QFileInfo> listBackups(const QString& backupsDir)
{
    QDir dir(backupsDir);
    return dir.entryInfoList({ "*.zip" }, QDir::Files, QDir::Time);
}

QList<QFileInfo> listBackups(BaseInstance* instance)
{
    return listBackups(backupsDirPath(instance));
}

int pruneBackups(const QString& backupsDir, int keepCount)
{
    if (keepCount < 1)
        keepCount = 1;

    const auto backups = listBackups(backupsDir);
    int removed = 0;
    for (int i = keepCount; i < backups.size(); ++i) {
        if (QFile::remove(backups.at(i).absoluteFilePath()))
            removed++;
    }
    return removed;
}

int pruneBackups(BaseInstance* instance, int keepCount)
{
    return pruneBackups(backupsDirPath(instance), keepCount);
}

QString createBackup(const QString& gameRoot, const QString& targetPath, QAtomicInt* cancel)
{
    if (!QFileInfo::exists(gameRoot))
        return QObject::tr("Instance game directory %1 does not exist").arg(gameRoot);

    QFileInfoList files;
    if (!MMCZip::collectFileListRecursively(gameRoot, nullptr, &files, nullptr))
        return QObject::tr("Failed to enumerate files in %1").arg(gameRoot);

    MMCZip::ArchiveWriter zip(targetPath);
    if (!zip.open()) {
        FS::deletePath(targetPath);
        return QObject::tr("Failed to create backup archive %1").arg(targetPath);
    }

    QDir directory(gameRoot);
    bool ok = true;
    bool canceled = false;
    for (const auto& file : files) {
        if (cancel && cancel->loadAcquire()) {
            canceled = true;
            break;
        }
        if (!zip.addFile(file.absoluteFilePath(), directory.relativeFilePath(file.absoluteFilePath()))) {
            ok = false;
            break;
        }
    }
    zip.close();
    if (canceled || !ok) {
        FS::deletePath(targetPath);
        if (canceled)
            return {};
        return QObject::tr("Failed to write backup archive %1").arg(targetPath);
    }
    return {};
}

QString createBackup(BaseInstance* instance, const QString& targetPath, QAtomicInt* cancel)
{
    return createBackup(instance->gameRoot(), targetPath, cancel);
}

}  // namespace InstanceBackup

QString BackupInstanceTask::doWork()
{
    if (!m_force && m_minIntervalHours > 0) {
        const auto backups = InstanceBackup::listBackups(m_instance);
        if (!backups.isEmpty()) {
            auto newest = backups.first().lastModified();
            if (newest.secsTo(QDateTime::currentDateTime()) < m_minIntervalHours * 3600) {
                m_skipped = true;
                return {};
            }
        }
    }

    QString dirPath = InstanceBackup::backupsDirPath(m_instance);
    if (!FS::ensureFolderPathExists(dirPath))
        return tr("Failed to create backups directory %1").arg(dirPath);

    QString target = FS::PathCombine(dirPath, InstanceBackup::generateBackupFileName(m_instance));
    QString error = InstanceBackup::createBackup(m_instance, target, &m_cancel);
    if (m_cancel.loadAcquire())
        return {};
    if (error.isEmpty()) {
        m_createdPath = target;
        InstanceBackup::pruneBackups(m_instance, m_keepCount);
    }
    return error;
}

void BackupInstanceTask::executeTask()
{
    setStatus(tr("Backing up instance %1...").arg(m_instance->name()));
    m_future = QtConcurrent::run(QThreadPool::globalInstance(), [this] { return doWork(); });
    connect(&m_watcher, &QFutureWatcher<QString>::finished, this, [this] {
        if (m_cancel.loadAcquire()) {
            emitAborted();
            return;
        }
        QString error = m_future.result();
        if (!error.isEmpty())
            emitFailed(error);
        else
            emitSucceeded();
    });
    m_watcher.setFuture(m_future);
}

bool BackupInstanceTask::abort()
{
    if (!m_future.isRunning())
        return false;
    m_cancel.storeRelease(1);
    m_future.waitForFinished();
    return true;
}
