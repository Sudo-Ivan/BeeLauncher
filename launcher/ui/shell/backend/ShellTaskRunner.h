#pragma once
#include <QObject>
#include <QPointer>
#include <QString>

class Task;

// Runs one Task at a time for the QML shell and mirrors its progress,
// status text and terminal state as QML friendly properties and signals.
// The runner takes ownership of each submitted task.
class ShellTaskRunner : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
   public:
    explicit ShellTaskRunner(QObject* parent = nullptr);

    bool busy() const { return m_busy; }
    // 0..1 while running, -1 when the task does not report a total.
    qreal progress() const { return m_progress; }
    QString statusText() const { return m_status; }

    // Parents the task to this runner and starts it. If a task is already
    // running the new task is deleted and taskFailed is emitted.
    // Used by the shell backends; intentionally not Q_INVOKABLE.
    void runTask(Task* task);

    Q_INVOKABLE void abort();

   signals:
    void taskFinished();
    void taskFailed(QString reason);
    void busyChanged();
    void progressChanged();
    void statusTextChanged();

   private:
    QPointer<Task> m_task;
    bool m_busy = false;
    qreal m_progress = -1;
    QString m_status;
};
