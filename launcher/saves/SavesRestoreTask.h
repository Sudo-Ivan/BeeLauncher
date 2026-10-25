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

#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QString>

#include <tasks/Task.h>

#include "SavesClient.h"
#include "SavesManifest.h"

/// Restores one snapshot into a fresh folder inside the saves directory.
class SavesRestoreTask : public Task {
    Q_OBJECT

   public:
    SavesRestoreTask(const QString& savesDir, const QString& slot, const QString& snapshotId, SavesClient* client = nullptr);
    ~SavesRestoreTask() override = default;

    bool canAbort() const override { return true; }

    QString restoredDirName() const { return m_targetName; }

   protected:
    void executeTask() override;
    bool abort() override;

   private:
    void processNext();
    void processFile(int index);
    void fetchNextChunk(int index);
    void finishFile(int index);
    void finishWithError(const QString& error);
    void finishRestore();

   private:
    QString m_savesDir;
    QString m_slot;
    QString m_snapshotId;
    SavesClient* m_client = nullptr;

    QString m_tempPath;
    QString m_targetName;
    Saves::ParsedManifest m_manifest;

    QFile* m_out = nullptr;
    int m_current = -1;
    int m_nextChunk = 0;
    int m_issued = 0;
    int m_inFlight = 0;
    QHash<int, QByteArray> m_pendingChunks;
    qint64 m_written = 0;
    qint64 m_downloadedBytes = 0;
    qint64 m_totalBytes = 0;
    bool m_aborted = false;
};
