#pragma once
#include <QObject>
#include <QPointer>
#include <QVariantList>

#include "ShellResourceModel.h"

class QAbstractItemModel;
class BaseInstance;
class MinecraftInstance;
class SettingsObject;

// QML-facing bridge for the in-shell instance detail page. Wraps one
// BaseInstance at a time; setInstanceId() resolves it through the running
// Application. All members no-op safely when no instance is selected or the
// backend was never attached.
class ShellInstanceBridge : public QObject {
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
    explicit ShellInstanceBridge(QObject* parent = nullptr);

    // Connects the bridge to the running Application. Tests never call this.
    void attachBackend();

    QString instanceId() const { return m_id; }
    void setInstanceId(const QString& id);

    bool hasInstance() const { return !m_inst.isNull(); }
    bool isMinecraft() const { return !m_mc.isNull(); }
    QString name() const;
    QString versionText() const;
    QString iconSource() const;
    bool running() const;
    QString typeDisplay() const;
    QString instanceRoot() const;
    QString gameRoot() const;
    QString notes() const;
    void setNotes(const QString& text);

    QAbstractItemModel* modsModel() const { return m_mods; }
    QAbstractItemModel* coreModsModel() const { return m_coreMods; }
    QAbstractItemModel* resourcePacksModel() const { return m_resourcePacks; }
    QAbstractItemModel* texturePacksModel() const { return m_texturePacks; }
    QAbstractItemModel* shaderPacksModel() const { return m_shaderPacks; }
    QAbstractItemModel* dataPacksModel() const { return m_dataPacks; }
    QAbstractItemModel* worldsModel() const { return m_worlds; }

    Q_INVOKABLE void launch();
    Q_INVOKABLE void kill();
    Q_INVOKABLE void openFolder();
    Q_INVOKABLE void openGameFolder();
    Q_INVOKABLE void openClassic();
    Q_INVOKABLE void openClassicPage(const QString& page);
    Q_INVOKABLE void openFile(const QString& path);

    Q_INVOKABLE QVariantList screenshots();
    Q_INVOKABLE QString screenshotUrl(const QString& path);
    Q_INVOKABLE void openScreenshotsFolder();

    Q_INVOKABLE QVariantList logFiles();
    Q_INVOKABLE QString readLogTail(const QString& path, int maxLines);
    Q_INVOKABLE void openLog(const QString& path);

    Q_INVOKABLE QVariant instValue(const QString& key);
    Q_INVOKABLE bool setInstValue(const QString& key, const QVariant& value);
    Q_INVOKABLE bool instGate(const QString& gateKey);
    Q_INVOKABLE void setInstGate(const QString& gateKey, bool on);
    Q_INVOKABLE void resetInstValue(const QString& key);

   signals:
    void instanceChanged();
    void settingsChanged();
    void actionFailed(const QString& message);

   private:
    void resolve();
    static QVariant normalizeFor(const QVariant& current, const QVariant& value);

    QString m_id;
    QPointer<BaseInstance> m_inst;
    QPointer<MinecraftInstance> m_mc;
    QPointer<SettingsObject> m_settings;
    bool m_backend = false;

    ShellResourceModel* m_mods = nullptr;
    ShellResourceModel* m_coreMods = nullptr;
    ShellResourceModel* m_resourcePacks = nullptr;
    ShellResourceModel* m_texturePacks = nullptr;
    ShellResourceModel* m_shaderPacks = nullptr;
    ShellResourceModel* m_dataPacks = nullptr;
    ShellResourceModel* m_worlds = nullptr;
};
