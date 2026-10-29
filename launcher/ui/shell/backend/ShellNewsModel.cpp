#include "ShellNewsModel.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

#include "Application.h"
#include "DesktopServices.h"
#include "net/ApiDownload.h"
#include "net/NetJob.h"

namespace {
const QString kNewsUrl = QStringLiteral("https://launchercontent.mojang.com/v2/news.json");
const QString kContentBase = QStringLiteral("https://launchercontent.mojang.com");

QString imageUrl(const QJsonObject& entry)
{
    QString path = entry.value(QStringLiteral("newsPageImage")).toObject().value(QStringLiteral("url")).toString();
    if (path.isEmpty())
        path = entry.value(QStringLiteral("playPageImage")).toObject().value(QStringLiteral("url")).toString();
    if (path.isEmpty())
        return {};
    if (path.startsWith(QStringLiteral("http")))
        return path;
    return kContentBase + path;
}
}  // namespace

ShellNewsModel::ShellNewsModel(QObject* parent) : QAbstractListModel(parent) {}

void ShellNewsModel::attachBackend()
{
    m_backend = true;
}

int ShellNewsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant ShellNewsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const Entry& entry = m_entries.at(index.row());
    switch (role) {
        case TitleRole:
        case Qt::DisplayRole:
            return entry.title;
        case TagRole:
            return entry.tag;
        case CategoryRole:
            return entry.category;
        case DateRole:
            return entry.date;
        case TextRole:
            return entry.text;
        case ImageUrlRole:
            return entry.imageUrl;
        case LinkRole:
            return entry.link;
        default:
            return {};
    }
}

QHash<int, QByteArray> ShellNewsModel::roleNames() const
{
    return { { TitleRole, "title" }, { TagRole, "tag" },           { CategoryRole, "category" }, { DateRole, "date" },
             { TextRole, "text" },   { ImageUrlRole, "imageUrl" }, { LinkRole, "link" } };
}

void ShellNewsModel::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void ShellNewsModel::setLastError(const QString& error)
{
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}

void ShellNewsModel::refresh()
{
    ++m_generation;
    if (m_job) {
        m_job->abort();
        m_job.reset();
    }
    if (!m_backend) {
        setLastError(tr("News is not available"));
        emit fetchFailed(m_lastError);
        return;
    }

    auto netJob = makeShared<NetJob>(tr("Minecraft news"), APPLICATION->network());
    auto [action, response] = Net::ApiDownload::makeByteArray(QUrl(kNewsUrl));
    netJob->addNetAction(action);

    const int generation = m_generation;
    connect(netJob.get(), &NetJob::succeeded, this, [this, generation, response] {
        if (generation != m_generation)
            return;
        m_job.reset();
        parseEntries(*response);
    });
    connect(netJob.get(), &NetJob::failed, this, [this, generation](const QString& reason) {
        if (generation != m_generation)
            return;
        m_job.reset();
        setBusy(false);
        setLastError(reason);
        emit fetchFailed(reason);
    });
    connect(netJob.get(), &NetJob::aborted, this, [this, generation] {
        if (generation != m_generation)
            return;
        m_job.reset();
        setBusy(false);
    });

    m_job = netJob;
    setBusy(true);
    netJob->start();
}

void ShellNewsModel::parseEntries(const QByteArray& response)
{
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        setBusy(false);
        setLastError(parseError.errorString());
        emit fetchFailed(m_lastError);
        return;
    }

    QList<Entry> entries;
    QList<Entry> javaEntries;
    const QJsonArray items = doc.object().value(QStringLiteral("entries")).toArray();
    entries.reserve(items.size());
    for (const auto& value : items) {
        const auto obj = value.toObject();
        Entry entry;
        entry.title = obj.value(QStringLiteral("title")).toString();
        entry.tag = obj.value(QStringLiteral("tag")).toString();
        entry.category = obj.value(QStringLiteral("category")).toString();
        entry.date = obj.value(QStringLiteral("date")).toString();
        entry.text = obj.value(QStringLiteral("text")).toString();
        entry.imageUrl = imageUrl(obj);
        entry.link = obj.value(QStringLiteral("readMoreLink")).toString();
        entries.append(entry);

        // This launcher plays Java Edition, so prefer articles tagged for it.
        const QJsonArray types = obj.value(QStringLiteral("newsType")).toArray();
        for (const auto& type : types) {
            if (type.toString() == QStringLiteral("Java")) {
                javaEntries.append(entry);
                break;
            }
        }
    }

    beginResetModel();
    m_entries = javaEntries.isEmpty() ? entries : javaEntries;
    endResetModel();
    setBusy(false);
    setLastError({});
    emit countChanged();
}

void ShellNewsModel::openLink(const QString& url)
{
    const QUrl target(url);
    if (target.isValid() && (target.scheme() == QStringLiteral("https") || target.scheme() == QStringLiteral("http")))
        DesktopServices::openUrl(target);
}
