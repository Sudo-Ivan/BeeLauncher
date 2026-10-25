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

#include "SavesRestoreTask.h"

#include <QDir>
#include <QUuid>

#include "FileSystem.h"

SavesRestoreTask::SavesRestoreTask(const QString& savesDir, const QString& slot, const QString& snapshotId, SavesClient* client)
    : m_savesDir(savesDir), m_slot(slot), m_snapshotId(snapshotId)
{
    m_client = client ? client : new SavesClient(this);
}

void SavesRestoreTask::executeTask()
{
    setStatus(tr("Fetching the snapshot manifest"));
    m_client->getManifest(m_slot, m_snapshotId, [this](const QByteArray& data, const QString& error) {
        if (m_aborted || !isRunning())
            return;
        if (!error.isEmpty()) {
            finishWithError(error);
            return;
        }
        QString decodeError;
        if (!Saves::decodeManifest(data, &m_manifest, &decodeError)) {
            finishWithError(decodeError);
            return;
        }
        const QString root = QStringLiteral("world/");
        for (const QString& d : m_manifest.dirs) {
            if (!d.startsWith(root) || !Saves::validatePath(d.mid(root.size()))) {
                finishWithError(tr("Snapshot contains an invalid path: %1").arg(d));
                return;
            }
        }
        for (const Saves::ManifestFile& f : m_manifest.files) {
            if (!f.path.startsWith(root) || !Saves::validatePath(f.path.mid(root.size()))) {
                finishWithError(tr("Snapshot contains an invalid path: %1").arg(f.path));
                return;
            }
            m_totalBytes += f.size;
        }

        const QString tempName = QStringLiteral(".saves-restore-") + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        m_tempPath = FS::PathCombine(m_savesDir, tempName);
        if (!QDir().mkpath(m_tempPath)) {
            finishWithError(tr("Failed to create a temporary directory in %1").arg(m_savesDir));
            return;
        }

        for (const QString& d : m_manifest.dirs)
            QDir().mkpath(FS::PathCombine(m_tempPath, d.mid(root.size())));

        setStatus(tr("Downloading files"));
        setProgress(0, std::max<qint64>(m_totalBytes, 1));
        processNext();
    });
}

void SavesRestoreTask::processNext()
{
    ++m_current;
    if (m_current >= m_manifest.files.size()) {
        finishRestore();
        return;
    }
    processFile(m_current);
}

void SavesRestoreTask::processFile(int index)
{
    const Saves::ManifestFile& f = m_manifest.files.at(index);
    const QString rel = f.path.mid(QStringLiteral("world/").size());
    const QString dest = FS::PathCombine(m_tempPath, rel);

    if (!FS::ensureFilePathExists(dest)) {
        finishWithError(tr("Failed to create %1").arg(dest));
        return;
    }

    m_written = 0;
    m_nextChunk = 0;
    m_issued = 0;
    m_pendingChunks.clear();

    delete m_out;
    m_out = new QFile(dest);
    if (!m_out->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        finishWithError(tr("Failed to open %1").arg(dest));
        return;
    }

    if (!f.inlineData.isEmpty() || f.chunks.isEmpty()) {
        if (!f.inlineData.isEmpty() && f.inlineData.size() != f.size) {
            finishWithError(tr("Snapshot file %1 has inconsistent inline data").arg(f.path));
            return;
        }
        m_out->write(f.inlineData);
        finishFile(index);
        return;
    }

    fetchNextChunk(index);
}

void SavesRestoreTask::fetchNextChunk(int index)
{
    const Saves::ManifestFile& f = m_manifest.files.at(index);
    while (m_issued < f.chunks.size() && m_inFlight < 4) {
        const int ci = m_issued++;
        ++m_inFlight;
        m_client->getChunk(f.chunks.at(ci).hash, [this, index, ci](const QByteArray& data, const QString& error) {
            --m_inFlight;
            if (m_aborted || !isRunning())
                return;
            if (!error.isEmpty()) {
                finishWithError(error);
                return;
            }
            if (m_current != index)
                return;
            const Saves::ManifestFile& f = m_manifest.files.at(index);
            if (data.size() != f.chunks.at(ci).size) {
                finishWithError(tr("Chunk %1 has the wrong size").arg(f.chunks.at(ci).hash.left(12)));
                return;
            }
            m_pendingChunks.insert(ci, data);
            while (m_pendingChunks.contains(m_nextChunk)) {
                const QByteArray chunk = m_pendingChunks.take(m_nextChunk);
                m_out->write(chunk);
                m_written += chunk.size();
                m_downloadedBytes += chunk.size();
                ++m_nextChunk;
            }
            setProgress(m_downloadedBytes, std::max<qint64>(m_totalBytes, 1));
            if (m_nextChunk == f.chunks.size()) {
                finishFile(index);
                return;
            }
            fetchNextChunk(index);
        });
    }
}

void SavesRestoreTask::finishFile(int index)
{
    const Saves::ManifestFile& f = m_manifest.files.at(index);
    m_out->flush();
    if (m_out->size() != f.size) {
        finishWithError(tr("Restored file %1 has the wrong size").arg(f.path));
        return;
    }
    const QString rel = f.path.mid(QStringLiteral("world/").size());
    const QString dest = FS::PathCombine(m_tempPath, rel);
    m_out->close();
    delete m_out;
    m_out = nullptr;

    QFile file(dest);
    if (file.open(QIODevice::ReadWrite)) {
        file.setFileTime(QDateTime::fromMSecsSinceEpoch(f.mtimeNs / 1000000), QFileDevice::FileModificationTime);
    }

    if (f.chunks.isEmpty() && !f.inlineData.isEmpty())
        m_downloadedBytes += f.size;
    if (f.size == 0)
        setProgress(m_downloadedBytes, std::max<qint64>(m_totalBytes, 1));

    processNext();
}

void SavesRestoreTask::finishRestore()
{
    const QDir saves(m_savesDir);
    QString name = m_slot;
    int n = 0;
    if (saves.exists(name)) {
        name = m_slot + tr(" (restored)");
        while (saves.exists(name))
            name = m_slot + tr(" (restored %1)").arg(++n + 1);
    }
    const QString target = saves.absoluteFilePath(name);
    if (!QDir().rename(m_tempPath, target)) {
        finishWithError(tr("Failed to move the restored world into place"));
        return;
    }
    m_targetName = name;
    emitSucceeded();
}

void SavesRestoreTask::finishWithError(const QString& error)
{
    if (m_aborted || !isRunning())
        return;
    delete m_out;
    m_out = nullptr;
    if (!m_tempPath.isEmpty())
        FS::deletePath(m_tempPath);
    emitFailed(error);
}

bool SavesRestoreTask::abort()
{
    m_aborted = true;
    if (m_client)
        m_client->cancelAll();
    if (!m_tempPath.isEmpty())
        FS::deletePath(m_tempPath);
    return Task::abort();
}
