#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantList>

#include "tasks/Task.h"

class ShellTaskRunner;

// Flat list model exposing version rows to QML. Used for both Minecraft
// versions and mod loader versions; loader rows leave type empty.
class ShellVersionRowModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
   public:
    enum Roles { DescriptorRole = Qt::UserRole + 1, TypeRole, RecommendedRole };

    struct Row {
        QString descriptor;
        QString type;
        bool recommended = false;
        qint64 sortKey = 0;
    };

    explicit ShellVersionRowModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(const QList<Row>& rows);
    QString descriptorAt(int row) const;
    int recommendedRow() const;

   signals:
    void countChanged();

   private:
    QList<Row> m_rows;
};

// Backend for the QML new instance dialog. Wraps the Meta::Index version
// lists, loader filtering and the VanillaCreationTask flow so QML never
// touches Application directly.
class ShellCreateController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* versionModel READ versionModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* loaderModel READ loaderModel CONSTANT)
    Q_PROPERTY(QString typeFilter READ typeFilter WRITE setTypeFilter NOTIFY typeFilterChanged)
    Q_PROPERTY(bool versionsBusy READ versionsBusy NOTIFY versionsBusyChanged)
    Q_PROPERTY(bool loadersBusy READ loadersBusy NOTIFY loadersBusyChanged)
   public:
    explicit ShellCreateController(QObject* parent = nullptr);

    // Connects the controller to the running Application. Tests never call this.
    void attachBackend();
    void setTaskRunner(ShellTaskRunner* runner);

    QAbstractItemModel* versionModel() const { return m_versionModel; }
    QAbstractItemModel* loaderModel() const { return m_loaderModel; }
    QString typeFilter() const { return m_typeFilter; }
    bool versionsBusy() const;
    bool loadersBusy() const;

    Q_INVOKABLE void loadVersions();
    Q_INVOKABLE void reload(bool network);
    Q_INVOKABLE void setTypeFilter(const QString& t);
    Q_INVOKABLE void loadLoaders(const QString& loaderUid, const QString& mcDescriptor);
    Q_INVOKABLE void create(const QString& name,
                            const QString& group,
                            const QString& mcDescriptor,
                            const QString& loaderUid,
                            const QString& loaderDescriptor);
    Q_INVOKABLE void copyInstance(const QString& instanceId, const QString& name, const QString& group);
    Q_INVOKABLE void renameInstance(const QString& instanceId, const QString& name);
    Q_INVOKABLE void openClassicWizard();
    Q_INVOKABLE QVariantList loaderOptions() const;

    // Row helpers for QML, which cannot call QAbstractItemModel methods directly.
    Q_INVOKABLE QString versionDescriptorAt(int row) const;
    Q_INVOKABLE QString loaderDescriptorAt(int row) const;
    Q_INVOKABLE int recommendedVersionRow() const;

   signals:
    void instanceCreated();
    void createFailed(QString reason);
    void versionsChanged();
    void loadersChanged();
    void versionsBusyChanged();
    void loadersBusyChanged();
    void typeFilterChanged();

   private:
    void rebuildVersionRows();
    void rebuildLoaderRows();
    static bool isEmptyParentLoader(const QString& uid);

    bool m_backend = false;
    ShellTaskRunner* m_runner = nullptr;
    bool m_createPending = false;
    ShellVersionRowModel* m_versionModel = nullptr;
    ShellVersionRowModel* m_loaderModel = nullptr;
    QString m_typeFilter = QStringLiteral("release");
    QString m_loaderUid;
    QString m_mcDescriptor;
    Task::Ptr m_versionLoadTask;
    Task::Ptr m_loaderLoadTask;
};
