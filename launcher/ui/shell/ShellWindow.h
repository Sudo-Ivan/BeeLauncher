#pragma once
#include <QObject>
#include <memory>

class QQmlApplicationEngine;
class QQuickWindow;
class ShellInstanceModel;
class ShellController;
class ShellSettings;
class ShellAccountBridge;
class ShellServerModel;
class ShellInstanceBridge;
class ShellTaskRunner;
class ShellCreateController;
class ShellDiscoverModel;

// Qt Quick main window host used when the UI mode is shell.
class ShellWindow : public QObject {
    Q_OBJECT
   public:
    explicit ShellWindow(QObject* parent = nullptr);
    ~ShellWindow() override;

    bool ok() const { return m_window != nullptr; }
    void show(bool minimized);
    void raiseWindow();
    ShellController* controller() const { return m_controller; }

   signals:
    void isClosing();

   private:
    void saveGeometry();
    void restoreGeometry();

    std::unique_ptr<QQmlApplicationEngine> m_engine;
    ShellInstanceModel* m_model = nullptr;
    ShellController* m_controller = nullptr;
    ShellSettings* m_settings = nullptr;
    ShellAccountBridge* m_accounts = nullptr;
    ShellServerModel* m_servers = nullptr;
    ShellInstanceBridge* m_instanceBridge = nullptr;
    ShellTaskRunner* m_taskRunner = nullptr;
    ShellCreateController* m_createController = nullptr;
    ShellDiscoverModel* m_discover = nullptr;
    QQuickWindow* m_window = nullptr;
};
