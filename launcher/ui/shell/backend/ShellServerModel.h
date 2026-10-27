#pragma once
#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QHash>
#include <QString>
#include <QVector>

class McClient;
class McResolver;
class QJsonObject;

// Saved-server list for the shell Servers tab. Entries persist to
// saved_servers.json under the data root. Live status fields are filled by
// Minecraft server-list pings (McResolver + McClient) on the GUI thread.
class ShellServerModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
   public:
    enum Roles {
        ServerIdRole = Qt::UserRole + 1,
        NameRole,
        AddressRole,
        MotdTextRole,
        VersionNameRole,
        OnlineRole,
        PlayersOnlineRole,
        PlayersMaxRole,
        LatencyMsRole,
        PingingRole,
        FaviconUrlRole,
    };

    explicit ShellServerModel(QObject* parent = nullptr);

    // Loads saved_servers.json. Tests never call this.
    void attachBackend();

    int count() const { return int(m_entries.size()); }
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addServer(const QString& name, const QString& address);
    Q_INVOKABLE void updateServer(int row, const QString& name, const QString& address);
    Q_INVOKABLE void removeServer(int row);
    Q_INVOKABLE void ping(int row);
    Q_INVOKABLE void pingAll();
    Q_INVOKABLE void join(int row, const QString& instanceId);
    Q_INVOKABLE void sortByName();

   signals:
    void errorOccurred(const QString& message);
    void countChanged();

   private:
    struct Entry {
        QString id;
        QString name;
        QString address;
        // live status, not persisted
        bool online = false;
        bool pinging = false;
        int playersOnline = -1;
        int playersMax = -1;
        qint64 latencyMs = -1;
        QString versionName;
        QString motdText;
        QString faviconUrl;
    };
    // One outstanding ping pipeline per server id.
    struct PingCtx {
        McResolver* resolver = nullptr;
        McClient* client = nullptr;
        QElapsedTimer timer;
    };

    bool validRow(int row) const { return row >= 0 && row < int(m_entries.size()); }
    int rowOf(const QString& id) const;
    void startPing(int row);
    void endPing(const QString& id);
    void applySuccess(const QString& id, const QJsonObject& data);
    void applyFailure(const QString& id, const QString& error);
    void emitRow(int row, const QVector<int>& roles);
    void clearStatus(Entry& e);
    void load();
    void save() const;
    QString storagePath() const;

    QVector<Entry> m_entries;
    QHash<QString, PingCtx*> m_pings;
    bool m_backend = false;
};
