#pragma once
#include <QObject>

// QML-facing controller. For stage 1 most actions delegate to the existing
// Widgets windows so the classic dialogs stay the single source of truth.
class ShellController : public QObject {
    Q_OBJECT
   public:
    explicit ShellController(QObject* parent = nullptr);
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
   signals:
    void toast(const QString& message);
    void busyChanged();
};
