#pragma once
#include <QObject>
#include <QQmlContext>
#include <QStandardItemModel>
#include <QQuickImageProvider>
#include <QPixmap>
#include <QColor>
#include <QHash>
#include <QPainter>
#include <QDateTime>
#include <QDir>
#include "InstanceList.h"

// Shared test/preview helpers. Not linked into the shipped app.

class SolidIconProvider : public QQuickImageProvider {
   public:
    SolidIconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override
    {
        int px = requestedSize.width() > 0 ? requestedSize.width() : 96;
        static const char* hues[] = { "#e8a33d", "#7c9f4b", "#4b8f9f", "#8f6fbf", "#bf5f5f" };
        int k = id.isEmpty() ? 0 : (id.at(0).toLatin1() % 5 + 5) % 5;
        QPixmap p(px, px);
        p.fill(Qt::transparent);
        QPainter painter(&p);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(hues[k]));
        painter.drawRoundedRect(QRectF(0, 0, px, px), px * 0.18, px * 0.18);
        painter.setPen(Qt::white);
        QFont f = painter.font();
        f.setPixelSize(px * 0.45);
        f.setBold(true);
        painter.setFont(f);
        painter.drawText(QRectF(0, 0, px, px), Qt::AlignCenter, id.isEmpty() ? "?" : QString(id.at(0).toUpper()));
        if (size) *size = p.size();
        return p;
    }
};

class MockShellController : public QObject {
    Q_OBJECT
   public:
    using QObject::QObject;
    Q_INVOKABLE void launch(const QString& id) { emit toast("launch " + id); }
    Q_INVOKABLE void kill(const QString& id) { emit toast("kill " + id); }
    Q_INVOKABLE void edit(const QString& id) { emit toast("edit " + id); }
    Q_INVOKABLE void openFolder(const QString& id) { emit toast("folder " + id); }
    Q_INVOKABLE void copyInstance(const QString& id) { emit toast("copy " + id); }
    Q_INVOKABLE void deleteInstance(const QString& id) { emit toast("delete " + id); }
    Q_INVOKABLE void createShortcut(const QString& id) { emit toast("shortcut " + id); }
    Q_INVOKABLE void changeGroup(const QString& id, const QString& g) { emit toast("group " + id); }
    Q_INVOKABLE void backupNow(const QString& id) { emit toast("backup " + id); }
    Q_INVOKABLE void repair(const QString& id) { emit toast("repair " + id); }
    Q_INVOKABLE void addInstance() { emit toast("addInstance"); }
    Q_INVOKABLE void openSettings() { emit toast("openSettings"); }
    Q_INVOKABLE void openAccounts() { emit toast("openAccounts"); }
   signals:
    void toast(const QString& message);
};

inline void fillMockInstances(QStandardItemModel& m)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    struct Row { const char* id; const char* name; const char* group; const char* ver; qint64 last; qint64 total; const char* icon; };
    const Row rows[] = {
        { "fabulously-optimized", "Fabulously Optimized", "Modded", "1.21.4 Fabric", now - 5 * 60000, 47 * 3600000LL, "fab" },
        { "create-modpack", "Create Modpack", "Modded", "1.20.1 Forge", now - 3 * 3600000LL, 12 * 3600000LL, "create" },
        { "skyfactory", "SkyFactory 4", "Modded", "1.12.2 Forge", 0, 0, "sky" },
        { "vanilla-1214", "Vanilla 1.21.4", "", "1.21.4", now - 4 * 24 * 3600000LL, 3 * 3600000LL, "vanilla" },
        { "lightly-modded", "Lightly Modded", "", "1.20.1 Fabric", 0, 900000LL, "lightly" },
        { "speedrun", "Speedrun Practice", "", "1.16.1", now - 30 * 60000, 8 * 3600000LL, "speed" },
    };
    for (auto& r : rows) {
        auto* it = new QStandardItem(r.name);
        it->setData(r.id, InstanceList::InstanceIDRole);
        it->setData(r.group, InstanceList::GroupRole);
        it->setData(r.icon, Qt::UserRole + 20);
        it->setData(r.last, Qt::UserRole + 21);
        it->setData(r.total, Qt::UserRole + 22);
        it->setData(r.ver, Qt::UserRole + 24);
        m.appendRow(it);
    }
}

class FileIconProvider : public QQuickImageProvider {
   public:
    FileIconProvider(const QString& dir) : QQuickImageProvider(QQuickImageProvider::Pixmap), m_dir(dir) {}
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override
    {
        QString path = m_dir + "/" + id + ".png";
        QPixmap p(path);
        if (p.isNull()) { p = QPixmap(requestedSize.isValid() ? requestedSize : QSize(96,96)); p.fill(QColor("#555")); }
        if (requestedSize.isValid()) p = p.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (size) *size = p.size();
        return p;
    }
   private:
    QString m_dir;
};

// Mock context objects matching the live shell backends. Registering these
// lets Main.qml instantiate outside the real app.
class MockShellSettings : public QObject {
    Q_OBJECT
   public:
    using QObject::QObject;
    Q_INVOKABLE QVariant value(const QString&) const { return {}; }
    Q_INVOKABLE bool setValue(const QString&, const QVariant&) { return true; }
    Q_INVOKABLE void resetValue(const QString&) {}
    Q_INVOKABLE QString dataRoot() const { return QStringLiteral("/tmp/bee-mock"); }
    Q_INVOKABLE bool openSettingPath(const QString&) { return false; }
    Q_INVOKABLE bool openPath(const QString&) { return false; }
    Q_INVOKABLE QVariantList iconThemes() const { return { QVariantMap{ { "id", "fluent_dark" }, { "name", "Fluent Dark" } } }; }
    Q_INVOKABLE QVariantList appThemes() const { return { QVariantMap{ { "id", "bee" }, { "name", "Bee" } } }; }
    Q_INVOKABLE QString appVersion() const { return QStringLiteral("0.0-test"); }
    Q_INVOKABLE QString qtVersion() const { return QString::fromLatin1(qVersion()); }
    Q_INVOKABLE QString gitRef() const { return QStringLiteral("test"); }
    Q_INVOKABLE void useClassicUi() {}
   signals:
    void changed(const QString& key);
};

class MockAccountsModel : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool msaBusy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool elybyBusy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool msaSupported READ msaSupported CONSTANT)
   public:
    using QStandardItemModel::QStandardItemModel;
    enum Roles { ProfileNameRole = Qt::UserRole + 1, TypeIdRole, TypeDisplayRole, StatusTextRole, StatusOkRole, IsDefaultRole, IsWorkingRole, FaceSourceRole };
    bool busy() const { return false; }
    bool msaSupported() const { return true; }
    QHash<int, QByteArray> roleNames() const override
    {
        return { { ProfileNameRole, "profileName" }, { TypeIdRole, "typeId" },         { TypeDisplayRole, "typeDisplay" },
                 { StatusTextRole, "statusText" },   { StatusOkRole, "statusOk" },   { IsDefaultRole, "isDefault" },
                 { IsWorkingRole, "isWorking" },     { FaceSourceRole, "faceSource" } };
    }
    Q_INVOKABLE void setDefault(int) {}
    Q_INVOKABLE void refresh(int) {}
    Q_INVOKABLE void remove(int row) { if (row >= 0 && row < rowCount()) removeRow(row); }
    Q_INVOKABLE void addMicrosoft() {}
    Q_INVOKABLE void cancelMicrosoft() {}
    Q_INVOKABLE void addOffline(const QString&) {}
    Q_INVOKABLE void addElyby(const QString&, const QString&) {}
    Q_INVOKABLE void cancelElyby() {}
    Q_INVOKABLE void openClassic() {}
   signals:
    void countChanged();
    void busyChanged();
    void accountAdded();
    void msaCodeRequested(QString url, QString code);
    void msaStatus(QString status);
    void msaFailed(QString reason);
    void msaFinished();
    void elybyCodeRequested(QString url, QString code);
    void elybyStatus(QString status);
    void elybyFailed(QString reason);
    void elybyFinished();
};

class MockServersModel : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
   public:
    using QStandardItemModel::QStandardItemModel;
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
        FaviconUrlRole
    };
    QHash<int, QByteArray> roleNames() const override
    {
        return { { ServerIdRole, "serverId" },     { NameRole, "name" },           { AddressRole, "address" },
                 { MotdTextRole, "motdText" },     { VersionNameRole, "versionName" }, { OnlineRole, "online" },
                 { PlayersOnlineRole, "playersOnline" }, { PlayersMaxRole, "playersMax" },
                 { LatencyMsRole, "latencyMs" },   { PingingRole, "pinging" },     { FaviconUrlRole, "faviconUrl" } };
    }
    Q_INVOKABLE void addServer(const QString&, const QString&) {}
    Q_INVOKABLE void updateServer(int, const QString&, const QString&) {}
    Q_INVOKABLE void removeServer(int row) { if (row >= 0 && row < rowCount()) removeRow(row); }
    Q_INVOKABLE void ping(int) {}
    Q_INVOKABLE void pingAll() {}
    Q_INVOKABLE void join(int, const QString&) {}
    Q_INVOKABLE void sortByName() {}
   signals:
    void errorOccurred(const QString& message);
    void countChanged();
};

class MockDiscoverModel : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
   public:
    using QStandardItemModel::QStandardItemModel;
    enum Roles { NameRole = Qt::UserRole + 1, SlugRole, DescriptionRole, AuthorRole, DownloadsRole, IconUrlRole, CategoriesTextRole };
    bool searching() const { return false; }
    QHash<int, QByteArray> roleNames() const override
    {
        return { { NameRole, "name" },           { SlugRole, "slug" },         { DescriptionRole, "description" },
                 { AuthorRole, "author" },       { DownloadsRole, "downloads" }, { IconUrlRole, "iconUrl" },
                 { CategoriesTextRole, "categoriesText" } };
    }
    Q_INVOKABLE void search(const QString&) {}
    Q_INVOKABLE void install(int) {}
   signals:
    void searchFinished();
    void searchFailed(QString reason);
    void installFinished();
    void installFailed(QString reason);
    void searchingChanged();
    void countChanged();
    void openClassicWizardRequested();
};

class MockNewsModel : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
   public:
    using QStandardItemModel::QStandardItemModel;
    enum Roles { TitleRole = Qt::UserRole + 1, TagRole, CategoryRole, DateRole, TextRole, ImageUrlRole, LinkRole };
    bool busy() const { return false; }
    QString lastError() const { return {}; }
    QHash<int, QByteArray> roleNames() const override
    {
        return { { TitleRole, "title" }, { TagRole, "tag" },         { CategoryRole, "category" }, { DateRole, "date" },
                 { TextRole, "text" },   { ImageUrlRole, "imageUrl" }, { LinkRole, "link" } };
    }
    Q_INVOKABLE void refresh() {}
    Q_INVOKABLE void openLink(const QString&) {}
   signals:
    void countChanged();
    void busyChanged();
    void lastErrorChanged();
    void fetchFailed(QString reason);
};

class MockInstanceBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString instanceId READ instanceId WRITE setInstanceId NOTIFY instanceChanged)
    Q_PROPERTY(bool hasInstance READ hasInstance NOTIFY instanceChanged)
    Q_PROPERTY(bool isMinecraft READ isMinecraft NOTIFY instanceChanged)
    Q_PROPERTY(QString name READ name NOTIFY instanceChanged)
    Q_PROPERTY(QString versionText READ versionText NOTIFY instanceChanged)
    Q_PROPERTY(QString iconSource READ iconSource NOTIFY instanceChanged)
    Q_PROPERTY(bool running READ running NOTIFY instanceChanged)
    Q_PROPERTY(QString typeDisplay READ typeDisplay NOTIFY instanceChanged)
    Q_PROPERTY(QString instanceRoot READ instanceRoot NOTIFY instanceChanged)
    Q_PROPERTY(QString gameRoot READ gameRoot NOTIFY instanceChanged)
    Q_PROPERTY(QString notes READ notes WRITE setNotes NOTIFY instanceChanged)
    Q_PROPERTY(QAbstractItemModel* modsModel READ modsModel NOTIFY instanceChanged)
    Q_PROPERTY(QAbstractItemModel* coreModsModel READ coreModsModel NOTIFY instanceChanged)
    Q_PROPERTY(QAbstractItemModel* resourcePacksModel READ resourcePacksModel NOTIFY instanceChanged)
    Q_PROPERTY(QAbstractItemModel* texturePacksModel READ texturePacksModel NOTIFY instanceChanged)
    Q_PROPERTY(QAbstractItemModel* shaderPacksModel READ shaderPacksModel NOTIFY instanceChanged)
    Q_PROPERTY(QAbstractItemModel* dataPacksModel READ dataPacksModel NOTIFY instanceChanged)
    Q_PROPERTY(QAbstractItemModel* worldsModel READ worldsModel NOTIFY instanceChanged)
   public:
    explicit MockInstanceBridge(QObject* parent = nullptr) : QObject(parent) {}
    QString instanceId() const { return m_id; }
    void setInstanceId(const QString& id) { m_id = id; emit instanceChanged(); }
    bool hasInstance() const { return !m_id.isEmpty(); }
    bool isMinecraft() const { return hasInstance(); }
    QString name() const { return m_id.isEmpty() ? QString() : QStringLiteral("Mock Instance"); }
    QString versionText() const { return hasInstance() ? QStringLiteral("1.21.4") : QString(); }
    QString iconSource() const { return QStringLiteral("image://beeicons/mock"); }
    bool running() const { return false; }
    QString typeDisplay() const { return QStringLiteral("Vanilla"); }
    QString instanceRoot() const { return QStringLiteral("/tmp/bee-mock/instances/") + m_id; }
    QString gameRoot() const { return instanceRoot() + QStringLiteral("/.minecraft"); }
    QString notes() const { return m_notes; }
    void setNotes(const QString& t) { m_notes = t; }
    QAbstractItemModel* modsModel() { return &m_empty; }
    QAbstractItemModel* coreModsModel() { return &m_empty; }
    QAbstractItemModel* resourcePacksModel() { return &m_empty; }
    QAbstractItemModel* texturePacksModel() { return &m_empty; }
    QAbstractItemModel* shaderPacksModel() { return &m_empty; }
    QAbstractItemModel* dataPacksModel() { return &m_empty; }
    QAbstractItemModel* worldsModel() { return &m_empty; }
    Q_INVOKABLE void launch() {}
    Q_INVOKABLE void kill() {}
    Q_INVOKABLE void openFolder() {}
    Q_INVOKABLE void openGameFolder() {}
    Q_INVOKABLE void openClassic() {}
    Q_INVOKABLE void openClassicPage(const QString&) {}
    Q_INVOKABLE void openFile(const QString&) {}
    Q_INVOKABLE QVariantList screenshots() { return {}; }
    Q_INVOKABLE QString screenshotUrl(const QString& path) { return QStringLiteral("file://") + path; }
    Q_INVOKABLE void openScreenshotsFolder() {}
    Q_INVOKABLE QVariantList logFiles() { return {}; }
    Q_INVOKABLE QString readLogTail(const QString&, int) { return QStringLiteral("[mock] empty log"); }
    Q_INVOKABLE void openLog(const QString&) {}
    Q_INVOKABLE QVariant instValue(const QString&) { return {}; }
    Q_INVOKABLE bool setInstValue(const QString&, const QVariant&) { return true; }
    Q_INVOKABLE bool instGate(const QString&) { return false; }
    Q_INVOKABLE void setInstGate(const QString&, bool) {}
    Q_INVOKABLE void resetInstValue(const QString&) {}
   signals:
    void instanceChanged();
    void settingsChanged();
    void actionFailed(const QString& message);
   private:
    QString m_id;
    QString m_notes;
    QStandardItemModel m_empty;
};

class MockTaskRunner : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
   public:
    using QObject::QObject;
    bool busy() const { return false; }
    qreal progress() const { return -1; }
    QString statusText() const { return QString(); }
    Q_INVOKABLE void abort() {}
   signals:
    void taskFinished();
    void taskFailed(QString reason);
    void busyChanged();
    void progressChanged();
    void statusTextChanged();
};

class MockCreateController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* versionModel READ versionModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* loaderModel READ loaderModel CONSTANT)
    Q_PROPERTY(QString typeFilter READ typeFilter WRITE setTypeFilter NOTIFY typeFilterChanged)
    Q_PROPERTY(bool versionsBusy READ versionsBusy NOTIFY versionsBusyChanged)
    Q_PROPERTY(bool loadersBusy READ loadersBusy NOTIFY loadersBusyChanged)
   public:
    explicit MockCreateController(QObject* parent = nullptr) : QObject(parent)
    {
        m_versions.setItemRoleNames({ { Qt::UserRole + 1, "descriptor" }, { Qt::UserRole + 2, "type" }, { Qt::UserRole + 3, "recommended" } });
        auto add = [&](const QString& d, const QString& t) {
            auto* it = new QStandardItem(d);
            it->setData(d, Qt::UserRole + 1);
            it->setData(t, Qt::UserRole + 2);
            m_versions.appendRow(it);
        };
        add("1.21.4", "release");
        add("1.21.3", "release");
        add("1.20.1", "release");
        add("25w04a", "snapshot");
    }
    QAbstractItemModel* versionModel() { return &m_versions; }
    QAbstractItemModel* loaderModel() { return &m_loaders; }
    QString typeFilter() const { return m_filter; }
    void setTypeFilter(const QString& t) { m_filter = t; }
    bool versionsBusy() const { return false; }
    bool loadersBusy() const { return false; }
    Q_INVOKABLE void loadVersions() {}
    Q_INVOKABLE void reload(bool) {}
    Q_INVOKABLE void loadLoaders(const QString&, const QString&) {}
    Q_INVOKABLE void create(const QString&, const QString&, const QString&, const QString&, const QString&) { emit instanceCreated(); }
    Q_INVOKABLE void openClassicWizard() {}
    Q_INVOKABLE QVariantList loaderOptions() const { return { QVariantMap{ { "uid", "" }, { "name", "Vanilla" } } }; }
    Q_INVOKABLE QString versionDescriptorAt(int row) const
    {
        return row >= 0 && row < m_versions.rowCount() ? m_versions.item(row)->data(Qt::UserRole + 1).toString() : QString();
    }
    Q_INVOKABLE QString loaderDescriptorAt(int row) const
    {
        return row >= 0 && row < m_loaders.rowCount() ? m_loaders.item(row)->data(Qt::UserRole + 1).toString() : QString();
    }
    Q_INVOKABLE int recommendedVersionRow() const { return 0; }
   signals:
    void instanceCreated();
    void createFailed(QString reason);
    void versionsChanged();
    void loadersChanged();
    void versionsBusyChanged();
    void loadersBusyChanged();
    void typeFilterChanged();
   private:
    QStandardItemModel m_versions;
    QStandardItemModel m_loaders;
    QString m_filter;
};

// Registers every context property Main.qml expects. Call after setting
// "instances" and "controller".
inline void registerShellContextMocks(QQmlContext* ctx, QObject* owner)
{
    ctx->setContextProperty("shellSettings", new MockShellSettings(owner));
    ctx->setContextProperty("accounts", new MockAccountsModel(owner));
    ctx->setContextProperty("servers", new MockServersModel(owner));
    ctx->setContextProperty("instanceBridge", new MockInstanceBridge(owner));
    ctx->setContextProperty("taskRunner", new MockTaskRunner(owner));
    ctx->setContextProperty("createCtrl", new MockCreateController(owner));
    ctx->setContextProperty("discover", new MockDiscoverModel(owner));
    ctx->setContextProperty("news", new MockNewsModel(owner));
}

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
inline void fillModrinthInstances(QStandardItemModel& m, const QString& dir)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QStringList vanilla = { "Vanilla 1.21.4", "Vanilla 1.8.9" };
    for (const QString& f : QDir(dir).entryList(QStringList{ "*.json" }, QDir::Files)) {
        QFile jf(dir + "/" + f);
        if (!jf.open(QIODevice::ReadOnly)) continue;
        QJsonObject o = QJsonDocument::fromJson(jf.readAll()).object();
        auto* it = new QStandardItem(o.value("title").toString());
        it->setData(o.value("slug").toString(), InstanceList::InstanceIDRole);
        QString group = o.value("group").toString();
        it->setData(group, InstanceList::GroupRole);
        it->setData(o.value("slug").toString(), Qt::UserRole + 20);
        qint64 last = now - (qAbs(o.value("slug").toString().length() % 10) + 1) * 3600000LL * (1 + f.length() % 48);
        it->setData(last, Qt::UserRole + 21);
        it->setData(qint64((o.value("downloads").toDouble() / 1000000) * 3600000) % (60 * 3600000LL), Qt::UserRole + 22);
        it->setData(o.value("game_version").toString() + " " + o.value("loader").toString(), Qt::UserRole + 24);
        m.appendRow(it);
    }
}
