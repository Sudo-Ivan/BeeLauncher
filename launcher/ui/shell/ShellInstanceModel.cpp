#include "ShellInstanceModel.h"
#include "../../InstanceList.h"
#include "../../BaseInstance.h"

ShellInstanceModel::ShellInstanceModel(QAbstractItemModel* source, QObject* parent) : QSortFilterProxyModel(parent)
{
    if (source) {
        setSourceModel(source);
        connect(source, &QAbstractItemModel::rowsInserted, this, &ShellInstanceModel::refreshGroups);
        connect(source, &QAbstractItemModel::rowsRemoved, this, &ShellInstanceModel::refreshGroups);
        connect(source, &QAbstractItemModel::dataChanged, this, &ShellInstanceModel::refreshGroups);
        connect(source, &QAbstractItemModel::modelReset, this, &ShellInstanceModel::refreshGroups);
    }
    setDynamicSortFilter(true);
    sort(0);
}

void ShellInstanceModel::setFilterText(const QString& t)
{
    if (m_filter == t) return;
    m_filter = t;
    invalidateFilter();
    emit filterTextChanged();
}

void ShellInstanceModel::setSortMode(SortMode m)
{
    if (m_sortMode == m) return;
    m_sortMode = m;
    invalidate();
    sort(0);
    emit sortModeChanged();
}

QHash<int, QByteArray> ShellInstanceModel::roleNames() const
{
    return {
        { InstanceIdRole, "instanceId" },     { NameRole, "name" },
        { GroupRole2, "group" },              { IconSourceRole, "iconSource" },
        { VersionTextRole, "versionText" },   { LastPlayedMsRole, "lastPlayedMs" },
        { TotalPlayTimeMsRole, "totalPlayTimeMs" }, { RunningRole, "running" },
        { StatusTextRole, "statusText" },     { InstancePointerShellRole, "instancePointer" },
    };
}

QVariant ShellInstanceModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};
    QModelIndex src = mapToSource(index);
    switch (role) {
        case InstanceIdRole: return src.data(InstanceList::InstanceIDRole);
        case NameRole: return src.data(Qt::DisplayRole);
        case GroupRole2: return src.data(InstanceList::GroupRole);
        case InstancePointerShellRole: return src.data(InstanceList::InstancePointerRole);
        case IconSourceRole: {
            auto* inst = src.data(InstanceList::InstancePointerRole).value<BaseInstance*>();
            if (inst) return QString("image://beeicons/" + inst->iconKey());
            QVariant k = src.data(Qt::UserRole + 20);  // test hook: icon key
            return k.isValid() ? QString("image://beeicons/" + k.toString()) : QVariant();
        }
        case RunningRole: {
            auto* inst = src.data(InstanceList::InstancePointerRole).value<BaseInstance*>();
            if (inst) return inst->isRunning();
            QVariant v = src.data(Qt::UserRole + 23);
            return v.isValid() && v.toBool();
        }
        case TotalPlayTimeMsRole: {
            auto* inst = src.data(InstanceList::InstancePointerRole).value<BaseInstance*>();
            if (inst) return inst->totalTimePlayed() * 1000LL;
            QVariant v = src.data(Qt::UserRole + 22);
            return v.isValid() ? v : 0;
        }
        case LastPlayedMsRole: {
            auto* inst = src.data(InstanceList::InstancePointerRole).value<BaseInstance*>();
            if (inst) return inst->lastLaunch();
            QVariant v = src.data(Qt::UserRole + 21);
            return v.isValid() ? v : 0;
        }
        case VersionTextRole: {
            QVariant v = src.data(Qt::UserRole + 24);  // test hook
            if (v.isValid()) return v;
            return src.data(Qt::ToolTipRole).toString().section('\n', 0, 0);
        }
        case StatusTextRole: return RunningRole == role ? QVariant() : src.data(Qt::StatusTipRole);
        default: return {};
    }
}

QString ShellInstanceModel::instanceIdAt(int row) const
{
    return data(index(row, 0), InstanceIdRole).toString();
}

void ShellInstanceModel::setGroupFilter(const QString& g)
{
    if (m_groupFilter == g) return;
    m_groupFilter = g;
    invalidateFilter();
    emit groupFilterChanged();
}

QStringList ShellInstanceModel::groups() const
{
    QSet<QString> out;
    if (sourceModel())
        for (int i = 0; i < sourceModel()->rowCount(); i++)
            out.insert(sourceModel()->index(i, 0).data(InstanceList::GroupRole).toString());
    QStringList list = out.values();
    list.sort(Qt::CaseInsensitive);
    list.removeAll(QString());
    list.append(QString());
    return list;
}

void ShellInstanceModel::refreshGroups()
{
    emit groupsChanged();
    emit countChanged();
}

bool ShellInstanceModel::filterAcceptsRow(int row, const QModelIndex& parent) const
{
    QModelIndex idx = sourceModel()->index(row, 0, parent);
    if (!m_groupFilter.isNull() && idx.data(InstanceList::GroupRole).toString() != m_groupFilter) return false;
    if (m_filter.isEmpty()) return true;
    return idx.data(Qt::DisplayRole).toString().contains(m_filter, Qt::CaseInsensitive);
}

bool ShellInstanceModel::lessThan(const QModelIndex& l, const QModelIndex& r) const
{
    const QModelIndex lp = mapFromSource(l);
    const QModelIndex rp = mapFromSource(r);
    if (m_sortMode == LastPlayed)
        return data(lp, LastPlayedMsRole).toLongLong() > data(rp, LastPlayedMsRole).toLongLong();
    if (m_sortMode == PlayTime)
        return data(lp, TotalPlayTimeMsRole).toLongLong() > data(rp, TotalPlayTimeMsRole).toLongLong();
    return data(lp, NameRole).toString().localeAwareCompare(data(rp, NameRole).toString()) < 0;
}
