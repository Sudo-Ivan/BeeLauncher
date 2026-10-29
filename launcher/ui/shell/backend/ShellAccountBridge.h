#pragma once
#include <QAbstractListModel>

#include "minecraft/auth/AuthFlow.h"
#include "minecraft/auth/MinecraftAccount.h"

// QML-facing account model and auth-flow driver for the shell Accounts page.
// attachBackend() wires it to the running Application; tests never call it.
class ShellAccountBridge : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool msaBusy READ msaBusy NOTIFY busyChanged)
    Q_PROPERTY(bool elybyBusy READ elybyBusy NOTIFY busyChanged)
    Q_PROPERTY(bool msaSupported READ msaSupported CONSTANT)
   public:
    enum Roles {
        ProfileNameRole = Qt::UserRole + 1,
        TypeIdRole,
        TypeDisplayRole,
        StatusTextRole,
        StatusOkRole,
        IsDefaultRole,
        IsWorkingRole,
        FaceSourceRole,
    };

    explicit ShellAccountBridge(QObject* parent = nullptr);

    // Connects the bridge to the running Application. Tests never call this.
    void attachBackend();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool busy() const { return msaBusy() || elybyBusy(); }
    bool msaBusy() const { return m_msaPending > 0; }
    bool elybyBusy() const { return m_elybyPending > 0; }
    bool msaSupported() const;

    Q_INVOKABLE void setDefault(int row);
    Q_INVOKABLE void refresh(int row);
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void addMicrosoft();
    Q_INVOKABLE void cancelMicrosoft();
    Q_INVOKABLE void addOffline(const QString& name);
    // Elyby only supports OAuth sign-in (browser or device code), so the
    // username and password arguments are accepted for API symmetry but unused.
    Q_INVOKABLE void addElyby(const QString& username, const QString& password);
    Q_INVOKABLE void cancelElyby();
    Q_INVOKABLE void openClassic();

   signals:
    void countChanged();
    void busyChanged();
    void accountAdded();
    void msaCodeRequested(QString url, QString code);
    void msaStatus(QString status);
    void msaFailed(QString reason);
    void msaFinished();
    void elybyCodeRequested(QString url, QString code);
    void elybyStatus(QString status);
    void elybyFailed(QString reason);
    void elybyFinished();

   private:
    MinecraftAccountPtr accountAt(int row) const;
    void updateBusy();
    void msaSucceeded();
    void msaFlowFinished();
    void elybySucceeded();
    void elybyFlowFinished();
    void abortMsaFlows();
    void abortElybyFlows();

    bool m_backend = false;
    int m_revision = 0;

    MinecraftAccountPtr m_msaAccount;
    shared_qobject_ptr<AuthFlow> m_msaFlow;
    int m_msaPending = 0;
    bool m_msaDone = false;
    QString m_msaFailReason;

    MinecraftAccountPtr m_elybyAccount;
    shared_qobject_ptr<AuthFlow> m_elybyBrowserFlow;
    shared_qobject_ptr<AuthFlow> m_elybyDeviceFlow;
    int m_elybyPending = 0;
    bool m_elybyDone = false;
    QString m_elybyFailReason;
};
