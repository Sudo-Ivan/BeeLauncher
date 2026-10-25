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

#include "SavesPushTask.h"

#include <QFile>
#include <QSysInfo>
#include <QtConcurrent/QtConcurrent>

SavesPushTask::SavesPushTask(const QString& worldDir, const QString& slot, const QString& label, SavesClient* client)
    : m_worldDir(worldDir), m_slot(slot), m_label(label)
{
    if (client) {
        m_client = client;
    } else {
        m_client = new SavesClient(this);
        m_ownClient = true;
    }
    connect(&m_watcher, &QFutureWatcherBase::finished, this, &SavesPushTask::onScanFinished);
}

void SavesPushTask::executeTask()
{
    setStatus(tr("Scanning world files"));
    setProgress(0, 100);
    m_future = QtConcurrent::run([this] {
        return Saves::scanWorld(m_worldDir, &m_cancel, [this](qint64 done, qint64 total) {
            if (total > 0)
                QMetaObject::invokeMethod(this, [this, done, total] { setProgress(done, total); }, Qt::QueuedConnection);
        });
    });
    m_watcher.setFuture(m_future);
}

void SavesPushTask::onScanFinished()
{
    if (m_aborted || !isRunning())
        return;
    m_scan = m_future.result();
    if (!m_scan.ok()) {
        finishWithError(m_scan.error == QLatin1String("aborted") ? tr("Aborted") : m_scan.error);
        return;
    }
    setStatus(tr("Checking the remote world"));
    m_client->getSlot(m_slot, [this](const SavesClient::SlotInfo& slot, const QString& error) {
        if (!error.isEmpty()) {
            finishWithError(error);
            return;
        }
        onSlotReady(slot.headSnapshotId);
    });
}

void SavesPushTask::onSlotReady(const QString& head)
{
    const QString device = QSysInfo::machineHostName();
    const QString label = m_label.isEmpty() ? tr("Bee Launcher") : m_label;
    const QByteArray manifest = Saves::encodeManifest(m_scan.files, m_scan.dirs, head, device, label);
    setStatus(tr("Creating the snapshot"));
    m_client->beginSnapshot(m_slot, manifest, [this, head](const SavesClient::BeginInfo& info, const QString& error) {
        if (!error.isEmpty()) {
            finishWithError(error);
            return;
        }
        onBegin(info, head);
    });
}

bool SavesPushTask::readChunk(const QString& hash, QByteArray* out)
{
    const ChunkSource src = m_chunkSources.value(hash);
    if (src.file.isEmpty())
        return false;
    QFile file(src.file);
    if (!file.open(QIODevice::ReadOnly) || !file.seek(src.offset))
        return false;
    *out = file.read(src.size);
    return out->size() == src.size;
}

void SavesPushTask::onBegin(const SavesClient::BeginInfo& info, const QString& head)
{
    m_snapshotId = info.id;
    m_beginChunkUrl = info.chunkUrl;
    m_head = head;

    QHash<QString, ChunkSource> all;
    for (const Saves::ManifestFile& f : m_scan.files) {
        for (const Saves::ChunkRef& ref : f.chunks)
            all.insert(ref.hash, { f.localPath, ref.offset, ref.size });
    }
    for (const QString& hash : info.missing) {
        if (!all.contains(hash)) {
            finishWithError(tr("Chunk %1 is missing locally").arg(hash.left(12)));
            return;
        }
        m_chunkSources.insert(hash, all.value(hash));
        m_uploadQueue.enqueue(hash);
        m_uploadTotalBytes += all.value(hash).size;
    }

    if (m_uploadQueue.isEmpty()) {
        onUploadsDone();
        return;
    }
    setStatus(tr("Uploading chunks"));
    uploadNext();
}

void SavesPushTask::uploadNext()
{
    while (m_inFlight < 4 && !m_uploadQueue.isEmpty()) {
        const QString hash = m_uploadQueue.dequeue();
        QByteArray data;
        if (!readChunk(hash, &data)) {
            m_firstUploadError = tr("Failed to read chunk %1").arg(hash.left(12));
            uploadNext();
            return;
        }
        ++m_inFlight;
        m_client->putChunk(m_beginChunkUrl, hash, data, [this, hash, data](bool, const QString& error) {
            --m_inFlight;
            if (m_aborted || !isRunning())
                return;
            if (!error.isEmpty()) {
                if (m_firstUploadError.isEmpty())
                    m_firstUploadError = error;
                uploadNext();
                return;
            }
            ++m_uploaded;
            m_uploadedBytes += data.size();
            setProgress(m_uploadedBytes, m_uploadTotalBytes);
            uploadNext();
        });
    }
    if (m_uploadQueue.isEmpty() && m_inFlight == 0)
        onUploadsDone();
}

void SavesPushTask::onUploadsDone()
{
    if (!m_firstUploadError.isEmpty()) {
        finishWithError(m_firstUploadError);
        return;
    }
    doCommit(m_head, false);
}

void SavesPushTask::doCommit(const QString& head, bool retried)
{
    setStatus(tr("Publishing the snapshot"));
    m_client->commit(m_slot, m_snapshotId, head, [this, retried](const SavesClient::CommitInfo& info, const QString& error) {
        if (m_aborted || !isRunning())
            return;
        if (!error.isEmpty()) {
            finishWithError(error);
            return;
        }
        if (info.conflict) {
            if (retried) {
                finishWithError(tr("The remote world changed while uploading"));
                return;
            }
            doCommit(info.head, true);
            return;
        }
        m_summary = tr("Snapshot %1: %2 files, %3 bytes, %4 chunks uploaded, %5 deduplicated, %6 skipped")
                        .arg(m_snapshotId)
                        .arg(m_scan.files.size())
                        .arg(m_scan.totalBytes)
                        .arg(m_uploaded)
                        .arg(m_scan.totalChunks - m_uploaded)
                        .arg(m_scan.skipped.size());
        emitSucceeded();
    });
}

void SavesPushTask::finishWithError(const QString& error)
{
    if (m_aborted || !isRunning())
        return;
    emitFailed(error);
}

bool SavesPushTask::abort()
{
    m_aborted = true;
    m_cancel.storeRelaxed(1);
    if (m_client)
        m_client->cancelAll();
    return Task::abort();
}
