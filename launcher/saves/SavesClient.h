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
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include <functional>

class QNetworkAccessManager;

#ifndef Q_MOC_RUN
struct SavesMeInfo {
    QString tier;
    qint64 usedBytes = 0;
    qint64 quotaBytes = 0;
    int slotCount = 0;
    int snapshotCount = 0;
    QString scopes;
    bool anonymous = false;
    bool requireEncryption = false;
};

struct SavesSlotInfo {
    QString name;
    QString headSnapshotId;
    QDateTime createdAt;
    QDateTime updatedAt;
};

struct SavesSnapshotInfo {
    QString id;
    QString parentId;
    QString device;
    QString label;
    QString state;
    qint64 totalBytes = 0;
    int fileCount = 0;
    QDateTime createdAt;
    QDateTime committedAt;
};

struct SavesBeginInfo {
    QString id;
    QStringList missing;
    QString chunkUrl;
};

struct SavesCommitInfo {
    bool conflict = false;
    QString head;
};
#endif  // Q_MOC_RUN

/// Async client for the saves cloud backup API. All callbacks run on
/// the thread that owns the client.
class SavesClient : public QObject {
    Q_OBJECT

   public:
    using MeInfo = SavesMeInfo;
    using SlotInfo = SavesSlotInfo;
    using SnapshotInfo = SavesSnapshotInfo;
    using BeginInfo = SavesBeginInfo;
    using CommitInfo = SavesCommitInfo;

    template <typename T>
    using Done = std::function<void(const T& value, const QString& error)>;

    /// Uses the launcher network stack and the configured settings.
    explicit SavesClient(QObject* parent = nullptr);

    /// Explicit dependency injection for tests.
    SavesClient(QNetworkAccessManager* nam, const QString& baseUrl, const QString& token, QObject* parent = nullptr);

    void me(Done<MeInfo> done);
    void listSlots(Done<QList<SlotInfo>> done);
    void getSlot(const QString& slot, Done<SlotInfo> done);
    void listSnapshots(const QString& slot, Done<QList<SnapshotInfo>> done);
    void getManifest(const QString& slot, const QString& snapshotId, Done<QByteArray> done);
    void beginSnapshot(const QString& slot, const QByteArray& manifest, Done<BeginInfo> done);
    void putChunk(const QString& chunkUrlBase, const QString& hash, const QByteArray& data, Done<bool> done);
    void commit(const QString& slot, const QString& snapshotId, const QString& expectedHead, Done<CommitInfo> done);
    void getChunk(const QString& hash, Done<QByteArray> done);

    /// Aborts every in-flight request this client created.
    void cancelAll();

    static QString mapError(int httpStatus, const QByteArray& body, const QString& networkError);

   private:
    using ReplyHandler = std::function<void(int httpStatus, const QByteArray& body, const QString& error)>;

    void putChunkAttempt(const QString& chunkUrlBase, const QString& hash, const QByteArray& data, Done<bool> done, int attempt);
    void perform(const QString& method,
                 const QString& path,
                 const QByteArray& body,
                 const QList<QPair<QByteArray, QByteArray>>& headers,
                 ReplyHandler handler,
                 int attempt = 0);
    void listSnapshotsPage(const QString& slot, const QString& cursor, QList<SnapshotInfo> acc, Done<QList<SnapshotInfo>> done);

    QString url(const QString& path) const;
    bool secureEnough(QString* error) const;

   private:
    QNetworkAccessManager* m_nam;
    QString m_baseUrl;
    QString m_token;
    QSet<QNetworkReply*> m_replies;
    bool m_canceled = false;
};
