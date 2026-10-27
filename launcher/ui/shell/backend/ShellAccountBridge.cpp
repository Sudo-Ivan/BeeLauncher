#include "ShellAccountBridge.h"

#include <QMetaObject>
#include <QUrl>

#include "Application.h"
#include "DesktopServices.h"
#include "minecraft/auth/AccountList.h"
#include "ui/MainWindow.h"

ShellAccountBridge::ShellAccountBridge(QObject* parent) : QAbstractListModel(parent) {}

void ShellAccountBridge::attachBackend()
{
    m_backend = true;
    auto* accounts = APPLICATION->accounts();

    // listChanged also fires when a single account's data changes (the
    // accountChanged slot forwards to onListChanged), so a reset is correct.
    connect(accounts, &AccountList::listChanged, this, [this] {
        beginResetModel();
        endResetModel();
        emit countChanged();
    });

    // Per-row updates that do not go through listChanged.
    auto poke = [this] {
        const int rows = rowCount();
        if (rows > 0)
            emit dataChanged(index(0), index(rows - 1));
    };
    connect(accounts, &AccountList::dataChanged, this, poke);
    connect(accounts, &AccountList::defaultAccountChanged, this, poke);
    connect(accounts, &AccountList::activityChanged, this, poke);
    connect(accounts, &AccountList::listActivityChanged, this, poke);

    emit countChanged();
}

int ShellAccountBridge::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !m_backend)
        return 0;
    return APPLICATION->accounts()->count();
}

QHash<int, QByteArray> ShellAccountBridge::roleNames() const
{
    return {
        { ProfileNameRole, "profileName" }, { TypeIdRole, "typeId" },   { TypeDisplayRole, "typeDisplay" },
        { StatusTextRole, "statusText" },   { StatusOkRole, "statusOk" }, { IsDefaultRole, "isDefault" },
        { IsWorkingRole, "isWorking" },
    };
}

static QString typeDisplayName(const QString& type)
{
    if (type == "msa")
        return ShellAccountBridge::tr("Microsoft account");
    if (type == "offline")
        return ShellAccountBridge::tr("Offline account");
    if (type == "elyby")
        return ShellAccountBridge::tr("Ely.by account");
    return ShellAccountBridge::tr("Custom account");
}

static QString statusDisplayName(AccountState state)
{
    switch (state) {
        case AccountState::Unchecked:
            return ShellAccountBridge::tr("Unchecked");
        case AccountState::Offline:
            return ShellAccountBridge::tr("Offline");
        case AccountState::Online:
            return ShellAccountBridge::tr("Ready");
        case AccountState::Working:
            return ShellAccountBridge::tr("Working");
        case AccountState::Errored:
            return ShellAccountBridge::tr("Errored");
        case AccountState::Expired:
            return ShellAccountBridge::tr("Expired");
        case AccountState::Disabled:
            return ShellAccountBridge::tr("Disabled");
        case AccountState::Gone:
            return ShellAccountBridge::tr("Gone");
        default:
            return ShellAccountBridge::tr("Unknown");
    }
}

MinecraftAccountPtr ShellAccountBridge::accountAt(int row) const
{
    if (!m_backend || row < 0 || row >= APPLICATION->accounts()->count())
        return nullptr;
    return APPLICATION->accounts()->at(row);
}

QVariant ShellAccountBridge::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
        return {};

    auto account = accountAt(index.row());
    if (!account)
        return {};

    switch (role) {
        case ProfileNameRole:
            return account->profileName();
        case TypeIdRole:
            return account->typeString();
        case TypeDisplayRole:
            return typeDisplayName(account->typeString());
        case StatusTextRole:
            return statusDisplayName(account->accountState());
        case StatusOkRole: {
            // Same usable states the classic UI treats as healthy.
            const auto state = account->accountState();
            return state == AccountState::Unchecked || state == AccountState::Offline || state == AccountState::Working ||
                   state == AccountState::Online;
        }
        case IsDefaultRole:
            return APPLICATION->accounts()->defaultAccount() == account;
        case IsWorkingRole:
            return account->isActive() || account->accountState() == AccountState::Working;
        default:
            return {};
    }
}

bool ShellAccountBridge::msaSupported() const
{
    return m_backend && APPLICATION->capabilities().testFlag(Application::SupportsMSA);
}

void ShellAccountBridge::setDefault(int row)
{
    if (auto account = accountAt(row))
        APPLICATION->accounts()->setDefaultAccount(account);
}

void ShellAccountBridge::refresh(int row)
{
    if (auto account = accountAt(row))
        APPLICATION->accounts()->requestRefresh(account->internalId());
}

void ShellAccountBridge::remove(int row)
{
    auto* accounts = APPLICATION->accounts();
    if (!m_backend || row < 0 || row >= accounts->count())
        return;
    // removeAccount clears the default itself when the row was the default.
    accounts->removeAccount(accounts->index(row));
    accounts->saveList();
}

void ShellAccountBridge::updateBusy()
{
    emit busyChanged();
}

void ShellAccountBridge::addMicrosoft()
{
    if (!m_backend || m_msaPending > 0)
        return;

    m_msaAccount = MinecraftAccount::createBlankMSA();
    m_msaFlow.reset(new AuthFlow(m_msaAccount->accountData(), AuthFlow::Action::DeviceCode));
    m_msaPending = 1;
    m_msaDone = false;
    m_msaFailReason.clear();

    connect(m_msaFlow.get(), &Task::status, this, [this](const QString& status) { emit msaStatus(status); });
    connect(m_msaFlow.get(), &Task::succeeded, this, &ShellAccountBridge::msaSucceeded);
    connect(m_msaFlow.get(), &Task::failed, this, [this](const QString& reason) { m_msaFailReason = reason; });
    connect(m_msaFlow.get(), &Task::finished, this, &ShellAccountBridge::msaFlowFinished);
    connect(m_msaFlow.get(), &AuthFlow::authorizeWithBrowserWithExtra, this,
            [this](const QString& url, const QString& code, int) { emit msaCodeRequested(url, code); });
    // The device code flow should not emit this, but if a future variant does
    // the only headless option is to hand the URL to the browser.
    connect(m_msaFlow.get(), &AuthFlow::authorizeWithBrowser, this,
            [](const QUrl& url) { DesktopServices::openUrl(url); });

    updateBusy();
    QMetaObject::invokeMethod(m_msaFlow.get(), &Task::start, Qt::QueuedConnection);
}

void ShellAccountBridge::msaSucceeded()
{
    if (m_msaDone)
        return;
    m_msaDone = true;

    auto* accounts = APPLICATION->accounts();
    accounts->addAccount(m_msaAccount);
    if (accounts->count() == 1)
        accounts->setDefaultAccount(m_msaAccount);
    accounts->saveList();
    emit accountAdded();
    emit msaFinished();
}

void ShellAccountBridge::msaFlowFinished()
{
    m_msaPending = qMax(0, m_msaPending - 1);
    if (m_msaPending == 0) {
        if (!m_msaDone) {
            m_msaDone = true;
            emit msaFailed(m_msaFailReason);
        }
        m_msaFlow.reset();
        m_msaAccount.reset();
    }
    updateBusy();
}

void ShellAccountBridge::abortMsaFlows()
{
    if (m_msaFlow)
        m_msaFlow->abort();
}

void ShellAccountBridge::cancelMicrosoft()
{
    abortMsaFlows();
}

void ShellAccountBridge::addOffline(const QString& name)
{
    const auto trimmed = name.trimmed();
    if (!m_backend || trimmed.isEmpty())
        return;

    if (const MinecraftAccountPtr account = MinecraftAccount::createOffline(trimmed)) {
        account->login()->start();  // completes synchronously, like the classic page
        auto* accounts = APPLICATION->accounts();
        accounts->addAccount(account);
        if (accounts->count() == 1)
            accounts->setDefaultAccount(account);
        accounts->saveList();
        emit accountAdded();
    }
}

void ShellAccountBridge::addElyby(const QString& username, const QString& password)
{
    Q_UNUSED(username)
    Q_UNUSED(password)
    if (!m_backend || m_elybyPending > 0)
        return;

    // Same pairing as ElybyLoginDialog: a browser OAuth flow and a device code
    // flow race; whichever finishes first wins.
    m_elybyAccount = MinecraftAccount::createBlankElyby();
    m_elybyBrowserFlow = m_elybyAccount->login(false);
    m_elybyDeviceFlow.reset(new AuthFlow(m_elybyAccount->accountData(), AuthFlow::Action::DeviceCode));
    m_elybyPending = 2;
    m_elybyDone = false;
    m_elybyFailReason.clear();

    for (auto flow : { m_elybyBrowserFlow, m_elybyDeviceFlow }) {
        connect(flow.get(), &Task::status, this, [this](const QString& status) { emit elybyStatus(status); });
        connect(flow.get(), &Task::succeeded, this, &ShellAccountBridge::elybySucceeded);
        connect(flow.get(), &Task::failed, this, [this](const QString& reason) { m_elybyFailReason = reason; });
        connect(flow.get(), &Task::finished, this, &ShellAccountBridge::elybyFlowFinished);
        connect(flow.get(), &AuthFlow::authorizeWithBrowser, this,
                [](const QUrl& url) { DesktopServices::openUrl(url); });
        connect(flow.get(), &AuthFlow::authorizeWithBrowserWithExtra, this,
                [this](const QString& url, const QString& code, int) { emit elybyCodeRequested(url, code); });
    }

    updateBusy();
    QMetaObject::invokeMethod(m_elybyBrowserFlow.get(), &Task::start, Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_elybyDeviceFlow.get(), &Task::start, Qt::QueuedConnection);
}

void ShellAccountBridge::elybySucceeded()
{
    if (m_elybyDone)
        return;
    m_elybyDone = true;
    abortElybyFlows();

    auto* accounts = APPLICATION->accounts();
    accounts->addAccount(m_elybyAccount);
    if (accounts->count() == 1)
        accounts->setDefaultAccount(m_elybyAccount);
    accounts->saveList();
    emit accountAdded();
    emit elybyFinished();
}

void ShellAccountBridge::elybyFlowFinished()
{
    m_elybyPending = qMax(0, m_elybyPending - 1);
    if (m_elybyPending == 0) {
        if (!m_elybyDone) {
            m_elybyDone = true;
            emit elybyFailed(m_elybyFailReason);
        }
        m_elybyBrowserFlow.reset();
        m_elybyDeviceFlow.reset();
        m_elybyAccount.reset();
    }
    updateBusy();
}

void ShellAccountBridge::abortElybyFlows()
{
    if (m_elybyBrowserFlow)
        m_elybyBrowserFlow->abort();
    if (m_elybyDeviceFlow)
        m_elybyDeviceFlow->abort();
}

void ShellAccountBridge::cancelElyby()
{
    abortElybyFlows();
}

void ShellAccountBridge::openClassic()
{
    if (!m_backend)
        return;
    if (auto* window = APPLICATION->legacyWindow())
        QMetaObject::invokeMethod(window, "on_actionManageAccounts_triggered");
}
