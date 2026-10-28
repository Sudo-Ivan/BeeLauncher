#include "ShellCreateController.h"

#include <algorithm>

#include "Application.h"
#include "BaseVersionList.h"
#include "InstanceList.h"
#include "Version.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "InstanceCopyTask.h"
#include "minecraft/VanillaInstanceCreationTask.h"
#include "ui/MainWindow.h"

#include "ShellTaskRunner.h"

ShellVersionRowModel::ShellVersionRowModel(QObject* parent) : QAbstractListModel(parent) {}

int ShellVersionRowModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant ShellVersionRowModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row& row = m_rows.at(index.row());
    switch (role) {
        case DescriptorRole:
        case Qt::DisplayRole:
            return row.descriptor;
        case TypeRole:
            return row.type;
        case RecommendedRole:
            return row.recommended;
        default:
            return {};
    }
}

QHash<int, QByteArray> ShellVersionRowModel::roleNames() const
{
    return { { DescriptorRole, "descriptor" }, { TypeRole, "type" }, { RecommendedRole, "recommended" } };
}

void ShellVersionRowModel::setRows(const QList<Row>& rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
    emit countChanged();
}

QString ShellVersionRowModel::descriptorAt(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return {};
    return m_rows.at(row).descriptor;
}

int ShellVersionRowModel::recommendedRow() const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).recommended)
            return i;
    }
    return -1;
}

// Meta type strings already match the values QML filters on.
static QString mapVersionType(const QString& metaType)
{
    if (metaType == "release" || metaType == "snapshot" || metaType == "old_beta" || metaType == "old_alpha" ||
        metaType == "experiment")
        return metaType;
    return QStringLiteral("other");
}

ShellCreateController::ShellCreateController(QObject* parent) : QObject(parent)
{
    m_versionModel = new ShellVersionRowModel(this);
    m_loaderModel = new ShellVersionRowModel(this);
}

void ShellCreateController::attachBackend()
{
    m_backend = true;
}

void ShellCreateController::setTaskRunner(ShellTaskRunner* runner)
{
    if (m_runner == runner)
        return;
    if (m_runner)
        disconnect(m_runner, nullptr, this, nullptr);
    m_runner = runner;
    if (m_runner) {
        connect(m_runner, &ShellTaskRunner::taskFinished, this, [this] {
            if (m_createPending) {
                m_createPending = false;
                emit instanceCreated();
            }
        });
        connect(m_runner, &ShellTaskRunner::taskFailed, this, [this](const QString& reason) {
            if (m_createPending) {
                m_createPending = false;
                emit createFailed(reason);
            }
        });
    }
}

bool ShellCreateController::versionsBusy() const
{
    return m_versionLoadTask && m_versionLoadTask->isRunning();
}

bool ShellCreateController::loadersBusy() const
{
    return m_loaderLoadTask && m_loaderLoadTask->isRunning();
}

void ShellCreateController::loadVersions()
{
    reload(false);
}

void ShellCreateController::reload(bool network)
{
    if (!m_backend)
        return;
    if (versionsBusy())
        return;
    auto vlist = APPLICATION->metadataIndex()->get(QStringLiteral("net.minecraft"));
    if (!vlist)
        return;
    if (vlist->isLoaded() && !network) {
        rebuildVersionRows();
        return;
    }
    m_versionLoadTask = vlist->getLoadTask(network);
    connect(m_versionLoadTask.get(), &Task::succeeded, this, [this] { rebuildVersionRows(); });
    connect(m_versionLoadTask.get(), &Task::failed, this, [this](const QString&) { rebuildVersionRows(); });
    connect(m_versionLoadTask.get(), &Task::finished, this, [this] {
        m_versionLoadTask.reset();
        emit versionsBusyChanged();
    });
    if (!m_versionLoadTask->isRunning())
        m_versionLoadTask->start();
    emit versionsBusyChanged();
}

void ShellCreateController::setTypeFilter(const QString& t)
{
    const QString filter = (t == "release" || t == "snapshot" || t == "all") ? t : QStringLiteral("release");
    if (filter == m_typeFilter)
        return;
    m_typeFilter = filter;
    emit typeFilterChanged();
    rebuildVersionRows();
}

void ShellCreateController::rebuildVersionRows()
{
    QList<ShellVersionRowModel::Row> rows;
    if (m_backend) {
        auto vlist = APPLICATION->metadataIndex()->get(QStringLiteral("net.minecraft"));
        if (vlist && vlist->isLoaded()) {
            for (int i = 0; i < vlist->count(); ++i) {
                const auto idx = vlist->index(i, 0);
                const QString type = mapVersionType(vlist->data(idx, BaseVersionList::TypeRole).toString());
                if (m_typeFilter != "all" && type != m_typeFilter)
                    continue;
                ShellVersionRowModel::Row row;
                row.descriptor = vlist->data(idx, BaseVersionList::VersionRole).toString();
                row.type = type;
                row.recommended = vlist->data(idx, BaseVersionList::RecommendedRole).toBool();
                row.sortKey = vlist->data(idx, BaseVersionList::SortRole).toLongLong();
                rows.append(row);
            }
        }
    }
    m_versionModel->setRows(rows);
    emit versionsChanged();
}

bool ShellCreateController::isEmptyParentLoader(const QString& uid)
{
    // Fabric and Quilt loader versions apply to every supported Minecraft
    // version, so their ParentVersionRole is empty instead of a pinned version.
    return uid == "net.fabricmc.fabric-loader" || uid == "org.quiltmc.quilt-loader";
}

void ShellCreateController::loadLoaders(const QString& loaderUid, const QString& mcDescriptor)
{
    m_loaderUid = loaderUid;
    m_mcDescriptor = mcDescriptor;
    if (!m_backend || loaderUid.isEmpty() || mcDescriptor.isEmpty()) {
        m_loaderModel->setRows({});
        emit loadersChanged();
        return;
    }
    auto vlist = APPLICATION->metadataIndex()->get(loaderUid);
    if (!vlist)
        return;
    if (vlist->isLoaded()) {
        rebuildLoaderRows();
        return;
    }
    if (loadersBusy()) {
        // The running load rebuilds with the newest selection when it finishes.
        return;
    }
    m_loaderLoadTask = vlist->getLoadTask();
    connect(m_loaderLoadTask.get(), &Task::succeeded, this, [this] { rebuildLoaderRows(); });
    connect(m_loaderLoadTask.get(), &Task::failed, this, [this](const QString&) { rebuildLoaderRows(); });
    connect(m_loaderLoadTask.get(), &Task::finished, this, [this] {
        m_loaderLoadTask.reset();
        emit loadersBusyChanged();
        // The selection may have changed while another list was loading.
        if (m_backend && !m_loaderUid.isEmpty() && !m_mcDescriptor.isEmpty()) {
            auto current = APPLICATION->metadataIndex()->get(m_loaderUid);
            if (current && !current->isLoaded())
                QMetaObject::invokeMethod(this, [this] { loadLoaders(m_loaderUid, m_mcDescriptor); }, Qt::QueuedConnection);
        }
    });
    if (!m_loaderLoadTask->isRunning())
        m_loaderLoadTask->start();
    emit loadersBusyChanged();
}

void ShellCreateController::rebuildLoaderRows()
{
    QList<ShellVersionRowModel::Row> rows;
    if (m_backend && !m_loaderUid.isEmpty() && !m_mcDescriptor.isEmpty()) {
        auto vlist = APPLICATION->metadataIndex()->get(m_loaderUid);
        if (vlist && vlist->isLoaded()) {
            const bool emptyParent = isEmptyParentLoader(m_loaderUid);
            // Fabric/Quilt only support Minecraft 1.14 and newer.
            const bool supported = !emptyParent || Version(m_mcDescriptor) >= Version(QStringLiteral("1.14"));
            if (supported) {
                for (int i = 0; i < vlist->count(); ++i) {
                    const auto idx = vlist->index(i, 0);
                    const QString parent = vlist->data(idx, BaseVersionList::ParentVersionRole).toString();
                    const bool match = emptyParent ? parent.isEmpty() : parent == m_mcDescriptor;
                    if (!match)
                        continue;
                    ShellVersionRowModel::Row row;
                    row.descriptor = vlist->data(idx, BaseVersionList::VersionRole).toString();
                    row.recommended = vlist->data(idx, BaseVersionList::RecommendedRole).toBool();
                    row.sortKey = vlist->data(idx, BaseVersionList::SortRole).toLongLong();
                    rows.append(row);
                }
                // Rows arrive newest first; keep recommended entries on top.
                std::stable_sort(rows.begin(), rows.end(), [](const ShellVersionRowModel::Row& a,
                                                            const ShellVersionRowModel::Row& b) {
                    return a.recommended && !b.recommended;
                });
            }
        }
    }
    m_loaderModel->setRows(rows);
    emit loadersChanged();
}

void ShellCreateController::create(const QString& name,
                                   const QString& group,
                                   const QString& mcDescriptor,
                                   const QString& loaderUid,
                                   const QString& loaderDescriptor)
{
    if (!m_backend) {
        emit createFailed(tr("The launcher backend is not available"));
        return;
    }
    if (!m_runner) {
        emit createFailed(tr("No task runner is configured"));
        return;
    }
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        emit createFailed(tr("The instance name is empty"));
        return;
    }

    auto mcList = APPLICATION->metadataIndex()->get(QStringLiteral("net.minecraft"));
    BaseVersion::Ptr mcVersion = mcList ? mcList->findVersion(mcDescriptor) : nullptr;
    if (!mcVersion) {
        emit createFailed(tr("Unknown Minecraft version %1").arg(mcDescriptor));
        return;
    }

    VanillaCreationTask* task = nullptr;
    if (loaderUid.isEmpty()) {
        task = new VanillaCreationTask(mcVersion);
    } else {
        auto loaderList = APPLICATION->metadataIndex()->get(loaderUid);
        BaseVersion::Ptr loaderVersion = loaderList ? loaderList->findVersion(loaderDescriptor) : nullptr;
        if (!loaderVersion) {
            emit createFailed(tr("Unknown loader version %1").arg(loaderDescriptor));
            return;
        }
        task = new VanillaCreationTask(mcVersion, loaderUid, loaderVersion);
    }
    task->setName(trimmedName);
    task->setGroup(group);
    task->setIcon(QStringLiteral("default"));

    m_createPending = true;
    m_runner->runTask(APPLICATION->instances()->wrapInstanceTask(task));
}

void ShellCreateController::copyInstance(const QString& instanceId, const QString& name, const QString& group)
{
    if (!m_backend || !m_runner) {
        emit createFailed(tr("The launcher backend is not available"));
        return;
    }
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        emit createFailed(tr("The instance name is empty"));
        return;
    }
    auto* inst = APPLICATION->instances()->getInstanceById(instanceId);
    if (!inst) {
        emit createFailed(tr("The instance no longer exists"));
        return;
    }
    auto* task = new InstanceCopyTask(inst, InstanceCopyPrefs());
    task->setName(trimmedName);
    task->setGroup(group);
    task->setIcon(inst->iconKey());
    m_createPending = true;
    m_runner->runTask(APPLICATION->instances()->wrapInstanceTask(task));
}

void ShellCreateController::renameInstance(const QString& instanceId, const QString& name)
{
    if (!m_backend)
        return;
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        emit createFailed(tr("The instance name is empty"));
        return;
    }
    if (auto* inst = APPLICATION->instances()->getInstanceById(instanceId))
        inst->setName(trimmedName);
}

void ShellCreateController::openClassicWizard()
{
    if (!m_backend)
        return;
    if (auto* window = APPLICATION->legacyWindow())
        QMetaObject::invokeMethod(window, "on_actionAddInstance_triggered");
}

QVariantList ShellCreateController::loaderOptions() const
{
    return {
        QVariantMap{ { "uid", "" }, { "name", tr("Vanilla") } },
        QVariantMap{ { "uid", "net.fabricmc.fabric-loader" }, { "name", tr("Fabric") } },
        QVariantMap{ { "uid", "org.quiltmc.quilt-loader" }, { "name", tr("Quilt") } },
        QVariantMap{ { "uid", "net.neoforged" }, { "name", tr("NeoForge") } },
        QVariantMap{ { "uid", "net.minecraftforge" }, { "name", tr("Forge") } },
        QVariantMap{ { "uid", "com.mumfrey.liteloader" }, { "name", tr("LiteLoader") } },
    };
}

QString ShellCreateController::versionDescriptorAt(int row) const
{
    return m_versionModel->descriptorAt(row);
}

QString ShellCreateController::loaderDescriptorAt(int row) const
{
    return m_loaderModel->descriptorAt(row);
}

int ShellCreateController::recommendedVersionRow() const
{
    return m_versionModel->recommendedRow();
}
