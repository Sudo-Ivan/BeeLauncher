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
#include <QFuture>
#include <QFutureWatcher>
#include <QQueue>
#include <QString>
#include <QStringList>

#include <tasks/Task.h>

#include "SavesClient.h"
#include "SavesManifest.h"

/// Pushes one world directory to the saves server as a snapshot.
class SavesPushTask : public Task {
    Q_OBJECT

   public:
    SavesPushTask(const QString& worldDir, const QString& slot, const QString& label, SavesClient* client = nullptr);
    ~SavesPushTask() override = default;

    bool canAbort() const override { return true; }

    QString snapshotId() const { return m_snapshotId; }
    qint64 fileCount() const { return m_scan.files.size(); }
    qint64 totalBytes() const { return m_scan.totalBytes; }
    qint64 chunksUploaded() const { return m_uploaded; }
    qint64 chunksDeduped() const { return m_scan.totalChunks - m_uploaded; }
    QList<Saves::SkippedPath> skipped() const { return m_scan.skipped; }
    QString summary() const { return m_summary; }

   protected:
    void executeTask() override;
    bool abort() override;

   private:
    void onScanFinished();
    void onSlotReady(const QString& head);
    void onBegin(const SavesClient::BeginInfo& info, const QString& head);
    void uploadNext();
    void onUploadsDone();
    void doCommit(const QString& head, bool retried);
    void finishWithError(const QString& error);
    bool readChunk(const QString& hash, QByteArray* out);

   private:
    QString m_worldDir;
    QString m_slot;
    QString m_label;
    SavesClient* m_client = nullptr;
    bool m_ownClient = false;

    Saves::ScanResult m_scan;
    QString m_snapshotId;
    QString m_summary;

    struct ChunkSource {
        QString file;
        qint64 offset = 0;
        qint64 size = 0;
    };

    QQueue<QString> m_uploadQueue;
    QHash<QString, ChunkSource> m_chunkSources;
    QString m_beginChunkUrl;
    QString m_head;
    int m_inFlight = 0;
    qint64 m_uploaded = 0;
    qint64 m_uploadedBytes = 0;
    qint64 m_uploadTotalBytes = 0;
    QString m_firstUploadError;

    QAtomicInt m_cancel{ 0 };
    QFuture<Saves::ScanResult> m_future;
    QFutureWatcher<Saves::ScanResult> m_watcher;
    bool m_aborted = false;
};
