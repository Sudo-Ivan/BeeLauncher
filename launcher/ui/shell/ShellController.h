#pragma once
#include <QObject>

// QML-facing controller. Most actions delegate to the existing Widgets
// window logic so the classic dialogs stay the single source of truth.
class ShellController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString accountName READ accountName NOTIFY accountChanged)
    Q_PROPERTY(QString accountType READ accountType NOTIFY accountChanged)
   public:
    explicit ShellController(QObject* parent = nullptr);

    // Connects the controller to the running Application. Tests never call this.
    void attachBackend();

    QString accountName() const;
    QString accountType() const;

    Q_INVOKABLE void launch(const QString& id);
    Q_INVOKABLE void kill(const QString& id);
    Q_INVOKABLE void edit(const QString& id);
    Q_INVOKABLE void openFolder(const QString& id);
    Q_INVOKABLE void copyInstance(const QString& id);
    Q_INVOKABLE void deleteInstance(const QString& id);
    Q_INVOKABLE void createShortcut(const QString& id);
    Q_INVOKABLE void changeGroup(const QString& id, const QString& group);
    Q_INVOKABLE void backupNow(const QString& id);
    Q_INVOKABLE void repair(const QString& id);
    Q_INVOKABLE void addInstance();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void openAccounts();
    Q_INVOKABLE void useClassicInterface();
   signals:
    void toast(const QString& message);
    void busyChanged();
    void accountChanged();

   private:
    bool legacyAction(const QString& id, const char* slot);
    bool m_backend = false;
};
