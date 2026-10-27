#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>

#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "tasks/Task.h"

class ShellTaskRunner;

// Modrinth modpack search results and install flow for the QML Discover page.
class ShellDiscoverModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
   public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        SlugRole,
        DescriptionRole,
        AuthorRole,
        DownloadsRole,
        IconUrlRole,
        CategoriesTextRole
    };

    explicit ShellDiscoverModel(QObject* parent = nullptr);

    // Connects the model to the running Application. Tests never call this.
    void attachBackend();
    void setTaskRunner(ShellTaskRunner* runner);

    bool searching() const { return m_searching; }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void search(const QString& query);
    Q_INVOKABLE void install(int row);

   signals:
    void searchFinished();
    void searchFailed(QString reason);
    void installFinished();
    void installFailed(QString reason);
    void searchingChanged();
    void countChanged();
    // Emitted when the headless install path is not usable so the
    // orchestrator can route to the classic dialog instead.
    void openClassicWizardRequested();

   private:
    struct Entry {
        ModPlatform::IndexedPack::Ptr pack;
        QString name;
        QString slug;
        QString description;
        QString author;
        qlonglong downloads = 0;
        QString iconUrl;
        QString categoriesText;
    };

    void parseHits(const QByteArray& response);
    void startInstall(const ModPlatform::IndexedPack::Ptr& pack, const QVector<ModPlatform::IndexedVersion>& versions);
    void setSearching(bool searching);

    bool m_backend = false;
    ShellTaskRunner* m_runner = nullptr;
    bool m_installPending = false;
    bool m_searching = false;
    int m_searchGeneration = 0;
    QList<Entry> m_entries;
    Task::Ptr m_searchJob;
    Task::Ptr m_versionsJob;
    ModrinthAPI m_api;
};
