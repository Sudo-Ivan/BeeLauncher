#include "ShellServerModel.h"
#include "../../../Application.h"
#include "../../../BaseInstance.h"
#include "../../../InstanceList.h"
#include "../../../LaunchMode.h"
#include "../../../minecraft/launch/MinecraftTarget.h"
#include "../../pages/instance/McClient.h"
#include "../../pages/instance/McResolver.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUuid>
#include <algorithm>
#include <memory>

namespace {

// Flatten a MOTD chat object {"text":..., "extra":[...]} (or a plain string
// or array of components) into plain text.
QString flattenChat(const QJsonValue& v)
{
    if (v.isString())
        return v.toString();
    if (v.isArray()) {
        QString out;
        for (const auto& e : v.toArray())
            out += flattenChat(e);
        return out;
    }
    if (!v.isObject())
        return {};
    const QJsonObject o = v.toObject();
    QString out = o.value(QStringLiteral("text")).toString();
    for (const auto& e : o.value(QStringLiteral("extra")).toArray())
        out += flattenChat(e);
    return out;
}

// Drop section-sign color codes (0xA7 followed by any one char) and flatten
// line breaks so the MOTD elides cleanly on one line.
QString cleanMotd(QString s)
{
    QString out;
    out.reserve(s.size());
    for (int i = 0; i < s.size(); i++) {
        if (s.at(i) == QChar(0x00A7)) {
            i++;
            continue;
        }
        out.append(s.at(i));
    }
    out.replace(QLatin1Char('\n'), QLatin1Char(' '));
    return out.simplified();
}

QString shortenError(QString error)
{
    error = error.simplified();
    if (error.size() > 80)
        error = error.left(80) + QStringLiteral("...");
    return error;
}

static const QVector<int> kStatusRoles = { ShellServerModel::MotdTextRole,
                                           ShellServerModel::VersionNameRole,
                                           ShellServerModel::OnlineRole,
                                           ShellServerModel::PlayersOnlineRole,
                                           ShellServerModel::PlayersMaxRole,
                                           ShellServerModel::LatencyMsRole,
                                           ShellServerModel::PingingRole,
                                           ShellServerModel::FaviconUrlRole };

}  // namespace

ShellServerModel::ShellServerModel(QObject* parent) : QAbstractListModel(parent) {}

void ShellServerModel::attachBackend()
{
    m_backend = true;
    load();
}

int ShellServerModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QHash<int, QByteArray> ShellServerModel::roleNames() const
{
    return {
        { ServerIdRole, "serverId" },     { NameRole, "name" },
        { AddressRole, "address" },       { MotdTextRole, "motdText" },
        { VersionNameRole, "versionName" }, { OnlineRole, "online" },
        { PlayersOnlineRole, "playersOnline" }, { PlayersMaxRole, "playersMax" },
        { LatencyMsRole, "latencyMs" },   { PingingRole, "pinging" },
        { FaviconUrlRole, "faviconUrl" },
    };
}

QVariant ShellServerModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= int(m_entries.size()))
        return {};
    const Entry& e = m_entries.at(index.row());
    switch (role) {
        case ServerIdRole: return e.id;
        case NameRole: return e.name;
        case AddressRole: return e.address;
        case MotdTextRole: return e.motdText;
        case VersionNameRole: return e.versionName;
        case OnlineRole: return e.online;
        case PlayersOnlineRole: return e.playersOnline;
        case PlayersMaxRole: return e.playersMax;
        case LatencyMsRole: return e.latencyMs;
        case PingingRole: return e.pinging;
        case FaviconUrlRole: return e.faviconUrl;
        default: return {};
    }
}

QString ShellServerModel::storagePath() const
{
    return QDir(APPLICATION->dataRoot()).filePath(QStringLiteral("saved_servers.json"));
}

void ShellServerModel::load()
{
    QVector<Entry> loaded;
    QFile f(storagePath());
    if (f.open(QIODevice::ReadOnly)) {
        for (const auto& v : QJsonDocument::fromJson(f.readAll()).array()) {
            const auto o = v.toObject();
            Entry e;
            e.id = o.value(QStringLiteral("id")).toString();
            e.name = o.value(QStringLiteral("name")).toString();
            e.address = o.value(QStringLiteral("address")).toString();
            if (e.address.isEmpty())
                continue;
            if (e.id.isEmpty())
                e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            loaded.append(e);
        }
    }
    beginResetModel();
    m_entries = loaded;
    endResetModel();
    emit countChanged();
}

void ShellServerModel::save() const
{
    if (!m_backend)
        return;
    QDir().mkpath(APPLICATION->dataRoot());
    QJsonArray arr;
    for (const auto& e : m_entries) {
        QJsonObject o;
        o.insert(QStringLiteral("id"), e.id);
        o.insert(QStringLiteral("name"), e.name);
        o.insert(QStringLiteral("address"), e.address);
        arr.append(o);
    }
    QSaveFile f(storagePath());
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    f.commit();
}

int ShellServerModel::rowOf(const QString& id) const
{
    for (int i = 0; i < int(m_entries.size()); i++)
        if (m_entries.at(i).id == id)
            return i;
    return -1;
}

void ShellServerModel::emitRow(int row, const QVector<int>& roles)
{
    if (!validRow(row))
        return;
    const QModelIndex i = index(row, 0);
    emit dataChanged(i, i, roles);
}

void ShellServerModel::clearStatus(Entry& e)
{
    e.online = false;
    e.pinging = false;
    e.playersOnline = -1;
    e.playersMax = -1;
    e.latencyMs = -1;
    e.versionName.clear();
    e.motdText.clear();
    e.faviconUrl.clear();
}

void ShellServerModel::addServer(const QString& name, const QString& address)
{
    const QString n = name.trimmed();
    const QString a = address.trimmed();
    if (n.isEmpty() || a.isEmpty()) {
        emit errorOccurred(tr("Server name and address are required"));
        return;
    }
    Entry e;
    e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    e.name = n;
    e.address = a;
    const int row = int(m_entries.size());
    beginInsertRows({}, row, row);
    m_entries.append(e);
    endInsertRows();
    save();
    emit countChanged();
    startPing(row);
}

void ShellServerModel::updateServer(int row, const QString& name, const QString& address)
{
    if (!validRow(row))
        return;
    const QString n = name.trimmed();
    const QString a = address.trimmed();
    if (n.isEmpty() || a.isEmpty()) {
        emit errorOccurred(tr("Server name and address are required"));
        return;
    }
    Entry& e = m_entries[row];
    const bool addressChanged = e.address != a;
    e.name = n;
    e.address = a;
    emitRow(row, { NameRole, AddressRole });
    save();
    if (addressChanged) {
        // drop any outstanding ping, reset status, re-ping
        endPing(e.id);
        clearStatus(e);
        emitRow(row, kStatusRoles);
        startPing(row);
    }
}

void ShellServerModel::removeServer(int row)
{
    if (!validRow(row))
        return;
    endPing(m_entries[row].id);
    beginRemoveRows({}, row, row);
    m_entries.removeAt(row);
    endRemoveRows();
    save();
    emit countChanged();
}

void ShellServerModel::ping(int row)
{
    startPing(row);
}

void ShellServerModel::pingAll()
{
    for (int i = 0; i < int(m_entries.size()); i++)
        startPing(i);
}

void ShellServerModel::sortByName()
{
    if (m_entries.size() < 2)
        return;
    beginResetModel();
    std::sort(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });
    endResetModel();
    save();
}

void ShellServerModel::join(int row, const QString& instanceId)
{
    if (!m_backend || !validRow(row))
        return;
    BaseInstance* inst = APPLICATION->instances()->getInstanceById(instanceId);
    if (!inst) {
        emit errorOccurred(tr("Instance not found"));
        return;
    }
    auto target = std::make_shared<MinecraftTarget>(MinecraftTarget::parse(m_entries[row].address, false));
    APPLICATION->launch(inst, LaunchMode::Normal, target);
}

void ShellServerModel::startPing(int row)
{
    if (!validRow(row))
        return;
    Entry& e = m_entries[row];
    const QString id = e.id;
    if (m_pings.contains(id))
        return;
    // split on the last ':' only when a numeric port follows
    QString host = e.address.trimmed();
    int port = 25565;
    const int c = host.lastIndexOf(QLatin1Char(':'));
    if (c > 0) {
        bool ok = false;
        const int p = host.mid(c + 1).toInt(&ok);
        if (ok && p > 0 && p <= 65535) {
            port = p;
            host = host.left(c).trimmed();
        }
    }
    if (host.isEmpty()) {
        e.online = false;
        e.pinging = false;
        e.motdText = tr("Invalid address");
        emitRow(row, { OnlineRole, PingingRole, MotdTextRole });
        return;
    }
    auto* ctx = new PingCtx;
    ctx->timer.start();
    m_pings.insert(id, ctx);
    if (!e.pinging) {
        e.pinging = true;
        emitRow(row, { PingingRole });
    }

    auto* resolver = new McResolver(nullptr, host, port);
    ctx->resolver = resolver;
    connect(resolver, &McResolver::succeeded, this, [this, id, host](QString ip, int resolvedPort) {
        PingCtx* c = m_pings.value(id);
        if (!c)
            return;
        auto* client = new McClient(nullptr, host, ip, static_cast<uint16_t>(resolvedPort));
        c->client = client;
        connect(client, &McClient::succeeded, this, [this, id](QJsonObject data) { applySuccess(id, data); });
        connect(client, &McClient::failed, this, [this, id](QString error) { applyFailure(id, error); });
        connect(client, &McClient::finished, this, [this, id]() { endPing(id); });
        client->getStatusData();
    });
    connect(resolver, &McResolver::failed, this, [this, id](QString error) { applyFailure(id, error); });
    connect(resolver, &McResolver::finished, this, [this, id]() {
        PingCtx* c = m_pings.value(id);
        if (!c)
            return;
        if (c->resolver) {
            c->resolver->deleteLater();
            c->resolver = nullptr;
        }
        // resolve failed; nothing else is still in flight
        if (!c->client)
            endPing(id);
    });
    resolver->ping();
}

void ShellServerModel::endPing(const QString& id)
{
    PingCtx* ctx = m_pings.take(id);
    if (!ctx)
        return;
    if (ctx->resolver)
        ctx->resolver->deleteLater();
    if (ctx->client)
        ctx->client->deleteLater();
    delete ctx;
    const int row = rowOf(id);
    if (row >= 0 && m_entries[row].pinging) {
        m_entries[row].pinging = false;
        emitRow(row, { PingingRole });
    }
}

void ShellServerModel::applySuccess(const QString& id, const QJsonObject& data)
{
    const int row = rowOf(id);
    PingCtx* ctx = m_pings.value(id);
    if (row < 0 || !ctx)
        return;   // entry removed or ping abandoned while in flight
    Entry& e = m_entries[row];
    e.online = true;
    e.pinging = false;
    e.latencyMs = ctx->timer.elapsed();
    const QJsonObject players = data.value(QStringLiteral("players")).toObject();
    e.playersOnline = players.contains(QStringLiteral("online")) ? players.value(QStringLiteral("online")).toInt(-1) : -1;
    e.playersMax = players.contains(QStringLiteral("max")) ? players.value(QStringLiteral("max")).toInt(-1) : -1;
    e.versionName = data.value(QStringLiteral("version")).toObject().value(QStringLiteral("name")).toString();
    e.motdText = cleanMotd(flattenChat(data.value(QStringLiteral("description"))));
    const QString fav = data.value(QStringLiteral("favicon")).toString();
    e.faviconUrl = fav.startsWith(QStringLiteral("data:image")) ? fav : QString();
    emitRow(row, kStatusRoles);
}

void ShellServerModel::applyFailure(const QString& id, const QString& error)
{
    const int row = rowOf(id);
    PingCtx* ctx = m_pings.value(id);
    if (row < 0 || !ctx)
        return;
    Entry& e = m_entries[row];
    e.online = false;
    e.pinging = false;
    e.latencyMs = -1;
    e.playersOnline = -1;
    e.playersMax = -1;
    e.versionName.clear();
    e.faviconUrl.clear();
    const QString shortErr = shortenError(error);
    e.motdText = shortErr.isEmpty() ? tr("Ping failed") : shortErr;
    emitRow(row, kStatusRoles);
}
