#include "ShellResourceModel.h"

#include <QFileInfo>
#include <QLocale>

#include "../../../DesktopServices.h"
#include "../../../minecraft/WorldList.h"
#include "../../../minecraft/mod/Mod.h"
#include "../../../minecraft/mod/ResourceFolderModel.h"

ShellResourceModel::ShellResourceModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

ShellResourceModel::~ShellResourceModel()
{
    if (m_weWatch) {
        if (m_resources)
            m_resources->stopWatching();
        if (m_worlds)
            m_worlds->stopWatching();
    }
}

void ShellResourceModel::setSource(QAbstractItemModel* source)
{
    if (m_weWatch) {
        if (m_resources)
            m_resources->stopWatching();
        if (m_worlds)
            m_worlds->stopWatching();
        m_weWatch = false;
    }

    m_resources = qobject_cast<ResourceFolderModel*>(source);
    m_worlds = qobject_cast<WorldList*>(source);
    m_kind = m_resources ? Kind::Resources : (m_worlds ? Kind::Worlds : Kind::None);

    setSourceModel(source);

    if (source) {
        connect(source, &QAbstractItemModel::rowsInserted, this, &ShellResourceModel::countChanged, Qt::UniqueConnection);
        connect(source, &QAbstractItemModel::rowsRemoved, this, &ShellResourceModel::countChanged, Qt::UniqueConnection);
        connect(source, &QAbstractItemModel::modelReset, this, &ShellResourceModel::countChanged, Qt::UniqueConnection);
    }

    // startWatching() kicks off the initial update() scan on both model
    // types. ResourceFolderModel reports whether it actually started, so we
    // only stop watchers we own. WorldList guards internally and is left
    // running for the instance lifetime, matching the classic pages.
    if (m_resources) {
        m_weWatch = m_resources->startWatching();
    } else if (m_worlds) {
        m_worlds->startWatching();
        m_weWatch = false;
    } else {
        m_weWatch = false;
    }

    emit sourceChanged();
    emit countChanged();
}

bool ShellResourceModel::canToggle() const
{
    return m_kind == Kind::Resources;
}

bool ShellResourceModel::canDelete() const
{
    return m_kind != Kind::None;
}

QString ShellResourceModel::folderPath() const
{
    if (m_resources)
        return m_resources->dir().absolutePath();
    if (m_worlds)
        return m_worlds->dir().absolutePath();
    return {};
}

QHash<int, QByteArray> ShellResourceModel::roleNames() const
{
    return { { NameRole, "name" },       { FileNameRole, "fileName" },   { EnabledRole, "enabled" },
             { SizeBytesRole, "sizeBytes" }, { SizeTextRole, "sizeText" }, { LastModifiedRole, "lastModified" },
             { InfoRole, "info" },       { GameModeRole, "gameMode" },   { SeedRole, "seed" } };
}

QVariant ShellResourceModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};
    const QModelIndex src = mapToSource(index);
    if (!src.isValid())
        return {};
    const int row = src.row();

    if (m_kind == Kind::Resources) {
        const Resource& res = m_resources->at(row);
        switch (role) {
            case Qt::DisplayRole:
            case NameRole:
                return res.name();
            case FileNameRole:
                return res.fileinfo().fileName();
            case EnabledRole:
                return res.enabled();
            case SizeBytesRole:
                return QVariant::fromValue<qlonglong>(res.sizeInfo());
            case SizeTextRole:
                return res.sizeStr();
            case LastModifiedRole:
                return res.dateTimeChanged().toMSecsSinceEpoch();
            case InfoRole: {
                if (auto* mod = dynamic_cast<const Mod*>(&res)) {
                    if (!mod->version().isEmpty())
                        return mod->version();
                }
                return res.provider();
            }
            default:
                return {};
        }
    }

    if (m_kind == Kind::Worlds) {
        auto at = [&](int column) { return m_worlds->index(row, column); };
        switch (role) {
            case Qt::DisplayRole:
            case NameRole:
                return at(WorldList::NameColumn).data(Qt::DisplayRole);
            case FileNameRole:
                return QFileInfo(at(0).data(WorldList::FolderRole).toString()).fileName();
            case EnabledRole:
                return true;
            case SizeBytesRole:
                return at(0).data(WorldList::SizeRole);
            case SizeTextRole:
                return QLocale().formattedDataSize(at(0).data(WorldList::SizeRole).toLongLong());
            case LastModifiedRole:
                return at(0).data(WorldList::LastPlayedRole).toDateTime().toMSecsSinceEpoch();
            case InfoRole:
                return at(WorldList::InfoColumn).data(Qt::DisplayRole);
            case GameModeRole:
                return at(WorldList::GameModeColumn).data(Qt::DisplayRole);
            case SeedRole:
                return at(0).data(WorldList::SeedRole);
            default:
                return {};
        }
    }

    return {};
}

void ShellResourceModel::setEnabled(int row, bool on)
{
    if (m_kind != Kind::Resources || row < 0 || row >= rowCount())
        return;
    const QModelIndex src = mapToSource(index(row, 0));
    if (src.isValid())
        m_resources->setResourceEnabled({ src }, on ? EnableAction::ENABLE : EnableAction::DISABLE);
}

bool ShellResourceModel::remove(int row)
{
    if (row < 0 || row >= rowCount())
        return false;
    const QModelIndex src = mapToSource(index(row, 0));
    if (!src.isValid())
        return false;
    if (m_resources)
        return m_resources->deleteResources({ src });
    if (m_worlds)
        return m_worlds->deleteWorld(src.row());
    return false;
}

void ShellResourceModel::openFolder()
{
    const QString path = folderPath();
    if (!path.isEmpty())
        DesktopServices::openPath(path, true);
}

void ShellResourceModel::refresh()
{
    if (m_resources)
        m_resources->update();
    else if (m_worlds)
        m_worlds->update();
}
