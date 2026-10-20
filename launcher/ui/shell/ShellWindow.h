#pragma once
#include <QObject>
#include <memory>

class QQmlApplicationEngine;
class QQuickWindow;
class ShellInstanceModel;
class ShellController;

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
    QQuickWindow* m_window = nullptr;
};
