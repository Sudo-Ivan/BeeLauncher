#include "ShellDiscoverModel.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Application.h"
#include "InstanceImportTask.h"
#include "InstanceList.h"
#include "Json.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"
#include "net/ApiDownload.h"
#include "net/NetJob.h"

#include "ShellTaskRunner.h"

ShellDiscoverModel::ShellDiscoverModel(QObject* parent) : QAbstractListModel(parent) {}

void ShellDiscoverModel::attachBackend()
{
    m_backend = true;
}

void ShellDiscoverModel::setTaskRunner(ShellTaskRunner* runner)
{
    if (m_runner == runner)
        return;
    if (m_runner)
        disconnect(m_runner, nullptr, this, nullptr);
    m_runner = runner;
    if (m_runner) {
        connect(m_runner, &ShellTaskRunner::taskFinished, this, [this] {
            if (m_installPending) {
                m_installPending = false;
                emit installFinished();
            }
        });
        connect(m_runner, &ShellTaskRunner::taskFailed, this, [this](const QString& reason) {
            if (m_installPending) {
                m_installPending = false;
                emit installFailed(reason);
            }
        });
    }
}

int ShellDiscoverModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant ShellDiscoverModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const Entry& entry = m_entries.at(index.row());
    switch (role) {
        case NameRole:
        case Qt::DisplayRole:
            return entry.name;
        case SlugRole:
            return entry.slug;
        case DescriptionRole:
            return entry.description;
        case AuthorRole:
            return entry.author;
        case DownloadsRole:
            return QVariant::fromValue(entry.downloads);
        case IconUrlRole:
            return entry.iconUrl;
        case CategoriesTextRole:
            return entry.categoriesText;
        default:
            return {};
    }
}

QHash<int, QByteArray> ShellDiscoverModel::roleNames() const
{
    return { { NameRole, "name" },         { SlugRole, "slug" },   { DescriptionRole, "description" },
             { AuthorRole, "author" },     { DownloadsRole, "downloads" },
             { IconUrlRole, "iconUrl" },   { CategoriesTextRole, "categoriesText" } };
}

void ShellDiscoverModel::setSearching(bool searching)
{
    if (searching == m_searching)
        return;
    m_searching = searching;
    emit searchingChanged();
}

void ShellDiscoverModel::search(const QString& query)
{
    // Invalidate results from any previous request first, then stop it.
    ++m_searchGeneration;
    if (m_searchJob) {
        m_searchJob->abort();
        m_searchJob.reset();
    }
    if (!m_backend) {
        emit searchFailed(tr("Search is not available"));
        return;
    }

    ResourceAPI::SearchArgs args;
    args.type = ModPlatform::ResourceType::Modpack;
    args.offset = 0;
    const QString trimmed = query.trimmed();
    if (!trimmed.isEmpty())
        args.search = trimmed;
    ResourceAPI::SortingMethod sort;
    sort.name = QStringLiteral("relevance");
    args.sorting = sort;

    const auto url = m_api.getSearchURL(args);
    if (!url.has_value()) {
        emit searchFailed(tr("Could not build the search request"));
        return;
    }

    auto netJob = makeShared<NetJob>(tr("Modrinth modpack search"), APPLICATION->network());
    auto [action, response] = Net::ApiDownload::makeByteArray(QUrl(url.value()));
    netJob->addNetAction(action);

    const int generation = m_searchGeneration;
    connect(netJob.get(), &NetJob::succeeded, this, [this, generation, response] {
        if (generation != m_searchGeneration)
            return;
        m_searchJob.reset();
        parseHits(*response);
    });
    connect(netJob.get(), &NetJob::failed, this, [this, generation](const QString& reason) {
        if (generation != m_searchGeneration)
            return;
        m_searchJob.reset();
        setSearching(false);
        emit searchFailed(reason);
    });
    connect(netJob.get(), &NetJob::aborted, this, [this, generation] {
        if (generation != m_searchGeneration)
            return;
        m_searchJob.reset();
        setSearching(false);
    });

    m_searchJob = netJob;
    setSearching(true);
    netJob->start();
}

void ShellDiscoverModel::parseHits(const QByteArray& response)
{
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        setSearching(false);
        emit searchFailed(parseError.errorString());
        return;
    }

    const QJsonArray hits = doc.object().value(QStringLiteral("hits")).toArray();
    QList<Entry> entries;
    entries.reserve(hits.size());
    for (const auto& value : hits) {
        auto obj = value.toObject();
        Entry entry;
        entry.pack = std::make_shared<ModPlatform::IndexedPack>();
        try {
            Modrinth::loadIndexedPack(*entry.pack, obj);
        } catch (const JSONValidationError& err) {
            qWarning().nospace() << "Skipping broken Modrinth search hit: " << err.cause();
            continue;
        }
        entry.name = entry.pack->name;
        entry.slug = entry.pack->slug;
        entry.description = obj.value(QStringLiteral("description")).toString();
        entry.author = obj.value(QStringLiteral("author")).toString();
        entry.downloads = obj.value(QStringLiteral("downloads")).toVariant().toLongLong();
        entry.iconUrl = entry.pack->logoUrl;
        QStringList categories;
        for (const auto& category : obj.value(QStringLiteral("categories")).toArray())
            categories << category.toString();
        entry.categoriesText = categories.join(QStringLiteral(", "));
        entries.append(entry);
    }

    beginResetModel();
    m_entries = entries;
    endResetModel();
    setSearching(false);
    emit countChanged();
    emit searchFinished();
}

void ShellDiscoverModel::install(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;
    if (!m_backend || !m_runner) {
        emit installFailed(tr("Install is not available"));
        return;
    }
    if (m_versionsJob && m_versionsJob->isRunning()) {
        emit installFailed(tr("Another install is already being prepared"));
        return;
    }

    auto pack = m_entries.at(row).pack;
    ResourceAPI::Callback<QVector<ModPlatform::IndexedVersion>> callbacks;
    callbacks.on_succeed = [this, pack](QVector<ModPlatform::IndexedVersion>& versions) {
        m_versionsJob.reset();
        startInstall(pack, versions);
    };
    callbacks.on_fail = [this](const QString& reason, int) {
        m_versionsJob.reset();
        emit installFailed(reason);
    };
    callbacks.on_abort = [this] {
        m_versionsJob.reset();
        emit installFailed(tr("Aborted"));
    };

    m_versionsJob = m_api.getProjectVersions({ pack, {}, {}, ModPlatform::ResourceType::Modpack }, std::move(callbacks));
    if (!m_versionsJob) {
        emit installFailed(tr("Could not request versions for %1").arg(pack->name));
        return;
    }
    m_versionsJob->start();
}

void ShellDiscoverModel::startInstall(const ModPlatform::IndexedPack::Ptr& pack,
                                      const QVector<ModPlatform::IndexedVersion>& versions)
{
    if (versions.isEmpty()) {
        emit installFailed(tr("No versions are available for %1").arg(pack->name));
        return;
    }

    // getProjectVersions sorts newest first, so take the front entry.
    const ModPlatform::IndexedVersion& version = versions.first();
    QMap<QString, QString> extraInfo;
    extraInfo.insert(QStringLiteral("pack_id"), pack->addonId.toString());
    extraInfo.insert(QStringLiteral("pack_version_id"), version.fileId.toString());

    // Same flow as ModrinthPage: the import task downloads the mrpack and
    // runs ModrinthCreationTask internally on the staging path.
    auto* task = new InstanceImportTask(version.downloadUrl, true, nullptr, std::move(extraInfo));
    task->setName(pack->name);
    task->setGroup(QString());
    task->setIcon(QStringLiteral("default"));

    m_installPending = true;
    m_runner->runTask(APPLICATION->instances()->wrapInstanceTask(task));
}
