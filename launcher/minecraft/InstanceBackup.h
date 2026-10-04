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

#include <QAtomicInt>
#include <QFileInfo>
#include <QFuture>
#include <QFutureWatcher>
#include <QList>
#include <QString>
#include <tasks/Task.h>

class BaseInstance;

namespace InstanceBackup {

/// The directory where backups for this instance are stored
QString backupsDirPath(BaseInstance* instance);

/// A fresh backup file name: <instance-name>-<timestamp>.zip.
/// When a file with that name already exists in backupsDir a numeric suffix
/// is appended so an existing backup is never overwritten.
QString generateBackupFileName(const QString& instanceName, const QString& backupsDir);
QString generateBackupFileName(BaseInstance* instance);

/// All backup archives in the directory, newest first
QList<QFileInfo> listBackups(const QString& backupsDir);
QList<QFileInfo> listBackups(BaseInstance* instance);

/// Delete the oldest backups beyond keepCount. Returns the number removed.
int pruneBackups(const QString& backupsDir, int keepCount);
int pruneBackups(BaseInstance* instance, int keepCount);

/// Zip gameRoot into targetPath. Returns an error string, empty on success.
/// When cancel is set to nonzero, the operation stops early and the partial
/// archive is removed. Safe to call from a worker thread.
QString createBackup(const QString& gameRoot, const QString& targetPath, QAtomicInt* cancel = nullptr);
QString createBackup(BaseInstance* instance, const QString& targetPath, QAtomicInt* cancel = nullptr);

}  // namespace InstanceBackup

/// Creates one backup archive for the instance. When minIntervalHours is set
/// and a newer backup already exists, the task finishes without doing anything.
class BackupInstanceTask : public Task {
    Q_OBJECT

   public:
    BackupInstanceTask(BaseInstance* instance, int minIntervalHours, int keepCount, bool force = false)
        : m_instance(instance), m_minIntervalHours(minIntervalHours), m_keepCount(keepCount), m_force(force)
    {}
    virtual ~BackupInstanceTask() = default;

    bool canAbort() const override { return true; }
    QString createdPath() const { return m_createdPath; }
    bool wasSkipped() const { return m_skipped; }

   protected:
    void executeTask() override;
    bool abort() override;

   private:
    QString doWork();

   private:
    BaseInstance* m_instance;
    int m_minIntervalHours;
    int m_keepCount;
    bool m_force;
    bool m_skipped = false;
    QString m_createdPath;
    QAtomicInt m_cancel{ 0 };

    QFuture<QString> m_future;
    QFutureWatcher<QString> m_watcher;
};
