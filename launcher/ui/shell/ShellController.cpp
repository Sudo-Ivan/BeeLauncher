#include "ShellController.h"
#include "../../Application.h"
#include "../../BaseInstance.h"
#include "../../DesktopServices.h"
#include "../../InstanceList.h"
#include "../../minecraft/auth/AccountList.h"
#include "../MainWindow.h"

ShellController::ShellController(QObject* parent) : QObject(parent) {}

void ShellController::attachBackend()
{
    m_backend = true;
    auto* accounts = APPLICATION->accounts();
    auto notify = [this] {
        ++m_revision;
        emit accountChanged();
    };
    connect(accounts, &AccountList::defaultAccountChanged, this, notify);
    connect(accounts, &AccountList::listChanged, this, notify);
    connect(accounts, &AccountList::dataChanged, this, notify);
}

QString ShellController::accountName() const
{
    if (!m_backend)
        return QStringLiteral("BeeUser");
    auto account = APPLICATION->accounts()->defaultAccount();
    return account ? account->profileName() : tr("No account");
}

QString ShellController::accountType() const
{
    if (!m_backend)
        return tr("Offline account");
    auto account = APPLICATION->accounts()->defaultAccount();
    if (!account)
        return tr("Sign in from Accounts");
    const auto type = account->typeString();
    if (type == "msa")
        return tr("Microsoft account");
    if (type == "offline")
        return tr("Offline account");
    if (type == "elyby")
        return tr("Ely.by account");
    return tr("Custom account");
}

QString ShellController::accountFace() const
{
    if (!m_backend)
        return {};
    auto account = APPLICATION->accounts()->defaultAccount();
    if (!account)
        return {};
    // The revision query forces QML to refetch when account data changes,
    // e.g. after a skin refresh.
    return QStringLiteral("image://beeavatar/%1?v=%2").arg(account->internalId()).arg(m_revision);
}

static BaseInstance* inst(const QString& id)
{
    return APPLICATION->instances()->getInstanceById(id);
}

bool ShellController::legacyAction(const QString& id, const char* slot)
{
    if (!m_backend)
        return false;
    auto* window = APPLICATION->legacyWindow();
    if (!window)
        return false;
    if (!id.isEmpty())
        QMetaObject::invokeMethod(window, "setSelectedInstanceById", Q_ARG(QString, id));
    return QMetaObject::invokeMethod(window, slot);
}

void ShellController::launch(const QString& id)
{
    if (!m_backend)
        return;
    if (auto* i = inst(id))
        APPLICATION->launch(i);
}

void ShellController::kill(const QString& id)
{
    if (!legacyAction(id, "on_actionKillInstance_triggered"))
        emit toast(tr("Stop is unavailable right now"));
}

void ShellController::edit(const QString& id)
{
    if (!m_backend)
        return;
    if (auto* i = inst(id))
        APPLICATION->showInstanceWindow(i);
}

void ShellController::openFolder(const QString& id)
{
    if (!m_backend)
        return;
    if (auto* i = inst(id))
        DesktopServices::openPath(i->instanceRoot(), true);
}

void ShellController::changeGroup(const QString& id, const QString& group)
{
    if (!m_backend)
        return;
    if (auto* i = inst(id))
        APPLICATION->instances()->setInstanceGroup(i->id(), group);
}

void ShellController::copyInstance(const QString& id)
{
    legacyAction(id, "on_actionCopyInstance_triggered");
}

void ShellController::deleteInstance(const QString& id)
{
    legacyAction(id, "on_actionDeleteInstance_triggered");
}

void ShellController::createShortcut(const QString& id)
{
    legacyAction(id, "on_actionCreateInstanceShortcut_triggered");
}

void ShellController::repair(const QString& id)
{
    legacyAction(id, "on_actionRepairInstance_triggered");
}

void ShellController::backupNow(const QString& id)
{
    if (!m_backend)
        return;
    if (auto* i = inst(id))
        APPLICATION->showInstanceWindow(i, "backups");
}

void ShellController::addInstance()
{
    legacyAction({}, "on_actionAddInstance_triggered");
}

void ShellController::openSettings()
{
    legacyAction({}, "on_actionSettings_triggered");
}

void ShellController::openAccounts()
{
    legacyAction({}, "on_actionManageAccounts_triggered");
}

void ShellController::useClassicInterface()
{
    if (!m_backend)
        return;
    APPLICATION->settings()->set("UiMode", "classic");
    emit toast(tr("The classic interface will be used after you restart Bee Launcher"));
}
