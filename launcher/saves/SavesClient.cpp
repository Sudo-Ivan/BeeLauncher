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

#include "SavesClient.h"

#include <QCryptographicHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>

#include "Application.h"
#include "BuildConfig.h"
#include "SavesManifest.h"
#include "ServiceUrls.h"
#include "settings/SettingsObject.h"

namespace {

QJsonValue getCI(const QJsonObject& obj, const QString& key)
{
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        if (it.key().compare(key, Qt::CaseInsensitive) == 0)
            return it.value();
    }
    return QJsonValue();
}

QDateTime parseTime(const QJsonValue& v)
{
    if (!v.isString())
        return QDateTime();
    return QDateTime::fromString(v.toString(), Qt::ISODateWithMs);
}

}  // namespace

SavesClient::SavesClient(QObject* parent) : QObject(parent)
{
    m_nam = APPLICATION->network();
    auto s = APPLICATION->settings();
    m_baseUrl = ServiceUrls::savesBase(s->get("SavesURL").toString());
    m_token = s->get("SavesToken").toString();
}

SavesClient::SavesClient(QNetworkAccessManager* nam, const QString& baseUrl, const QString& token, QObject* parent)
    : QObject(parent), m_nam(nam), m_token(token)
{
    m_baseUrl = baseUrl;
    if (!m_baseUrl.endsWith('/'))
        m_baseUrl.append('/');
}

QString SavesClient::url(const QString& path) const
{
    if (path.startsWith(QStringLiteral("http://")) || path.startsWith(QStringLiteral("https://")))
        return path;
    QString p = path;
    if (p.startsWith('/'))
        p.remove(0, 1);
    return m_baseUrl + p;
}

bool SavesClient::secureEnough(QString* error) const
{
    if (m_token.isEmpty()) {
        if (error)
            *error = tr("No saves token set. Add one in Settings, Services.");
        return false;
    }
    const QUrl base(m_baseUrl);
    if (base.scheme() == QStringLiteral("http")) {
        const QString host = base.host();
        const bool loopback =
            host == QStringLiteral("localhost") || host.endsWith(QStringLiteral(".localhost")) || QHostAddress(host).isLoopback();
        if (!loopback) {
            if (error)
                *error = tr("Refusing to send the token over plain http");
            return false;
        }
    }
    return true;
}

void SavesClient::perform(const QString& method,
                          const QString& path,
                          const QByteArray& body,
                          const QList<QPair<QByteArray, QByteArray>>& headers,
                          ReplyHandler handler,
                          int attempt)
{
    QString guardError;
    if (!secureEnough(&guardError)) {
        handler(-1, QByteArray(), guardError);
        return;
    }
    if (m_canceled) {
        handler(-1, QByteArray(), tr("Aborted"));
        return;
    }

    QNetworkRequest request(QUrl(url(path)));
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(BuildConfig.USER_AGENT.toUtf8()));
    request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    for (const auto& h : headers)
        request.setRawHeader(h.first, h.second);

    QNetworkReply* reply = nullptr;
    if (method == QLatin1String("GET")) {
        reply = m_nam->get(request);
    } else if (method == QLatin1String("POST")) {
        if (!request.hasRawHeader("Content-Type"))
            request.setRawHeader("Content-Type", "application/json");
        reply = m_nam->post(request, body);
    } else if (method == QLatin1String("PUT")) {
        request.setHeader(QNetworkRequest::ContentLengthHeader, body.size());
        reply = m_nam->put(request, body);
    } else {
        handler(-1, QByteArray(), tr("Unsupported method"));
        return;
    }

    m_replies.insert(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, method, path, body, headers, handler, attempt]() {
        m_replies.remove(reply);
        const QByteArray data = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto netError = reply->error();
        const QString netString = reply->errorString();
        const int retryAfter = reply->rawHeader("Retry-After").toInt();
        reply->deleteLater();

        if (m_canceled || netError == QNetworkReply::OperationCanceledError) {
            handler(-1, QByteArray(), tr("Aborted"));
            return;
        }
        if (status == 429 && attempt < 3) {
            const int waitMs = std::min(std::max(retryAfter, 1), 5) * 1000;
            QTimer::singleShot(waitMs, this, [this, method, path, body, headers, handler, attempt]() {
                perform(method, path, body, headers, handler, attempt + 1);
            });
            return;
        }
        if (netError != QNetworkReply::NoError && status == 0) {
            handler(0, QByteArray(), netString);
            return;
        }
        if (status >= 400) {
            handler(status, data, mapError(status, data, netString));
            return;
        }
        handler(status, data, QString());
    });
}

QString SavesClient::mapError(int httpStatus, const QByteArray& body, const QString& networkError)
{
    if (httpStatus == 0)
        return networkError.isEmpty() ? tr("Network error") : networkError;

    QString code;
    QString title;
    QString detail;
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        code = obj.value(QStringLiteral("code")).toString();
        title = obj.value(QStringLiteral("title")).toString();
        detail = obj.value(QStringLiteral("detail")).toString();
    }

    if (httpStatus == 401)
        return tr("Invalid or missing token");
    if (httpStatus == 413 || code == QStringLiteral("quota_exceeded"))
        return tr("Cloud storage quota exceeded");
    if (code == QStringLiteral("encryption_required"))
        return tr("This server requires client-side encryption (vault), which Bee Launcher does not support yet");
    if (code == QStringLiteral("invalid_manifest"))
        return detail.isEmpty() ? tr("Invalid manifest") : detail;
    if (code == QStringLiteral("not_found"))
        return tr("Not found");
    if (!title.isEmpty())
        return tr("%1 (status %2)").arg(title).arg(httpStatus);
    return tr("Request failed (status %1)").arg(httpStatus);
}

void SavesClient::cancelAll()
{
    m_canceled = true;
    const auto replies = m_replies;
    for (QNetworkReply* reply : replies)
        reply->abort();
}

void SavesClient::me(Done<MeInfo> done)
{
    perform(QStringLiteral("GET"), QStringLiteral("/v1/me"), {}, {}, [done](int status, const QByteArray& data, const QString& error) {
        if (!error.isEmpty()) {
            done(MeInfo{}, error);
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(data).object();
        MeInfo info;
        info.tier = getCI(obj, QStringLiteral("tier")).toString();
        info.scopes = getCI(obj, QStringLiteral("scopes")).toString();
        info.anonymous = getCI(obj, QStringLiteral("anonymous")).toBool();
        info.requireEncryption = getCI(obj, QStringLiteral("require_encryption")).toBool();
        const QJsonObject usage = getCI(obj, QStringLiteral("usage")).toObject();
        info.usedBytes = static_cast<qint64>(getCI(usage, QStringLiteral("used_bytes")).toDouble());
        info.quotaBytes = static_cast<qint64>(getCI(usage, QStringLiteral("quota_bytes")).toDouble());
        info.slotCount = getCI(usage, QStringLiteral("slots")).toInt();
        info.snapshotCount = getCI(usage, QStringLiteral("snapshots")).toInt();
        done(info, QString());
    });
}

static SavesClient::SlotInfo parseSlot(const QJsonObject& obj)
{
    SavesClient::SlotInfo info;
    info.name = getCI(obj, QStringLiteral("Name")).toString();
    info.headSnapshotId = getCI(obj, QStringLiteral("HeadSnapshotID")).toString();
    info.createdAt = parseTime(getCI(obj, QStringLiteral("CreatedAt")));
    info.updatedAt = parseTime(getCI(obj, QStringLiteral("UpdatedAt")));
    return info;
}

void SavesClient::listSlots(Done<QList<SlotInfo>> done)
{
    const QString path = QStringLiteral("/v1/apps/%1/slots").arg(QString::fromLatin1(Saves::APP_ID));
    perform(QStringLiteral("GET"), path, {}, {}, [done](int status, const QByteArray& data, const QString& error) {
        if (status == 404) {
            done(QList<SlotInfo>{}, QString());
            return;
        }
        if (!error.isEmpty()) {
            done(QList<SlotInfo>{}, error);
            return;
        }
        QList<SlotInfo> out;
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            for (const QJsonValue& v : doc.array()) {
                if (v.isObject())
                    out.append(parseSlot(v.toObject()));
            }
        }
        done(out, QString());
    });
}

void SavesClient::getSlot(const QString& slot, Done<SlotInfo> done)
{
    const QString path =
        QStringLiteral("/v1/apps/%1/slots/%2").arg(QString::fromLatin1(Saves::APP_ID), QString::fromUtf8(QUrl::toPercentEncoding(slot)));
    perform(QStringLiteral("GET"), path, {}, {}, [done](int status, const QByteArray& data, const QString& error) {
        if (status == 404) {
            done(SlotInfo{}, QString());
            return;
        }
        if (!error.isEmpty()) {
            done(SlotInfo{}, error);
            return;
        }
        done(parseSlot(QJsonDocument::fromJson(data).object()), QString());
    });
}

void SavesClient::listSnapshotsPage(const QString& slot, const QString& cursor, QList<SnapshotInfo> acc, Done<QList<SnapshotInfo>> done)
{
    QString path = QStringLiteral("/v1/apps/%1/slots/%2/snapshots?limit=50")
                       .arg(QString::fromLatin1(Saves::APP_ID), QString::fromUtf8(QUrl::toPercentEncoding(slot)));
    if (!cursor.isEmpty())
        path += QStringLiteral("&cursor=") + QString::fromUtf8(QUrl::toPercentEncoding(cursor));
    perform(QStringLiteral("GET"), path, {}, {}, [this, slot, acc, done](int status, const QByteArray& data, const QString& error) mutable {
        if (status == 404) {
            done(acc, QString());
            return;
        }
        if (!error.isEmpty()) {
            done(QList<SnapshotInfo>{}, error);
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(data).object();
        for (const QJsonValue& v : getCI(obj, QStringLiteral("snapshots")).toArray()) {
            const QJsonObject so = v.toObject();
            SnapshotInfo info;
            info.id = getCI(so, QStringLiteral("ID")).toString();
            info.parentId = getCI(so, QStringLiteral("ParentID")).toString();
            info.device = getCI(so, QStringLiteral("Device")).toString();
            info.label = getCI(so, QStringLiteral("Label")).toString();
            info.state = getCI(so, QStringLiteral("State")).toString();
            info.totalBytes = static_cast<qint64>(getCI(so, QStringLiteral("TotalBytes")).toDouble());
            info.fileCount = getCI(so, QStringLiteral("FileCount")).toInt();
            info.createdAt = parseTime(getCI(so, QStringLiteral("CreatedAt")));
            info.committedAt = parseTime(getCI(so, QStringLiteral("CommittedAt")));
            if (info.state == QStringLiteral("committed"))
                acc.append(info);
        }
        const QString next = getCI(obj, QStringLiteral("next")).toString();
        if (!next.isEmpty()) {
            listSnapshotsPage(slot, next, acc, done);
            return;
        }
        std::sort(acc.begin(), acc.end(), [](const SnapshotInfo& a, const SnapshotInfo& b) { return a.committedAt > b.committedAt; });
        done(acc, QString());
    });
}

void SavesClient::listSnapshots(const QString& slot, Done<QList<SnapshotInfo>> done)
{
    listSnapshotsPage(slot, QString(), {}, done);
}

void SavesClient::getManifest(const QString& slot, const QString& snapshotId, Done<QByteArray> done)
{
    const QString path = QStringLiteral("/v1/apps/%1/slots/%2/snapshots/%3")
                             .arg(QString::fromLatin1(Saves::APP_ID), QString::fromUtf8(QUrl::toPercentEncoding(slot)),
                                  QString::fromUtf8(QUrl::toPercentEncoding(snapshotId)));
    perform(QStringLiteral("GET"), path, {}, {}, [done](int, const QByteArray& data, const QString& error) { done(data, error); });
}

void SavesClient::beginSnapshot(const QString& slot, const QByteArray& manifest, Done<BeginInfo> done)
{
    const QString path = QStringLiteral("/v1/apps/%1/slots/%2/snapshots")
                             .arg(QString::fromLatin1(Saves::APP_ID), QString::fromUtf8(QUrl::toPercentEncoding(slot)));
    perform(QStringLiteral("POST"), path, manifest, {}, [done](int, const QByteArray& data, const QString& error) {
        if (!error.isEmpty()) {
            done(BeginInfo{}, error);
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(data).object();
        BeginInfo info;
        info.id = getCI(obj, QStringLiteral("id")).toString();
        info.chunkUrl = getCI(obj, QStringLiteral("chunk_url")).toString();
        for (const QJsonValue& v : getCI(obj, QStringLiteral("missing")).toArray())
            info.missing.append(v.toString());
        done(info, QString());
    });
}

void SavesClient::putChunk(const QString& chunkUrlBase, const QString& hash, const QByteArray& data, Done<bool> done)
{
    putChunkAttempt(chunkUrlBase, hash, data, done, 0);
}

void SavesClient::putChunkAttempt(const QString& chunkUrlBase, const QString& hash, const QByteArray& data, Done<bool> done, int attempt)
{
    perform(QStringLiteral("PUT"), chunkUrlBase + hash, data, {},
            [this, chunkUrlBase, hash, data, done, attempt](int status, const QByteArray&, const QString& error) {
                if (error.isEmpty()) {
                    done(true, QString());
                    return;
                }
                const bool retryable = status == 0 || status >= 500;
                if (retryable && attempt < 3) {
                    QTimer::singleShot(250 << attempt, this, [this, chunkUrlBase, hash, data, done, attempt]() {
                        putChunkAttempt(chunkUrlBase, hash, data, done, attempt + 1);
                    });
                    return;
                }
                done(false, error);
            });
}

void SavesClient::commit(const QString& slot, const QString& snapshotId, const QString& expectedHead, Done<CommitInfo> done)
{
    const QString path = QStringLiteral("/v1/apps/%1/slots/%2/snapshots/%3/commit")
                             .arg(QString::fromLatin1(Saves::APP_ID), QString::fromUtf8(QUrl::toPercentEncoding(slot)),
                                  QString::fromUtf8(QUrl::toPercentEncoding(snapshotId)));
    const QByteArray ifMatch = '"' + expectedHead.toUtf8() + '"';
    perform(QStringLiteral("POST"), path, {}, { { "If-Match", ifMatch } },
            [done](int status, const QByteArray& data, const QString& error) {
                if (status == 409) {
                    CommitInfo info;
                    info.conflict = true;
                    const QJsonValue head = getCI(QJsonDocument::fromJson(data).object(), QStringLiteral("head"));
                    info.head = head.isString() ? head.toString() : head.toObject().value(QStringLiteral("id")).toString();
                    done(info, QString());
                    return;
                }
                if (!error.isEmpty()) {
                    done(CommitInfo{}, error);
                    return;
                }
                done(CommitInfo{}, QString());
            });
}

void SavesClient::getChunk(const QString& hash, Done<QByteArray> done)
{
    const QString path = QStringLiteral("/v1/apps/%1/chunks/%2").arg(QString::fromLatin1(Saves::APP_ID), hash);
    perform(QStringLiteral("GET"), path, {}, {}, [hash, done](int, const QByteArray& data, const QString& error) {
        if (!error.isEmpty()) {
            done(QByteArray(), error);
            return;
        }
        const QByteArray actual = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
        if (actual != hash.toLatin1()) {
            done(QByteArray(), tr("Chunk %1 failed the integrity check").arg(hash.left(12)));
            return;
        }
        done(data, QString());
    });
}
