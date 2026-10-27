#include "ShellTaskRunner.h"

#include "tasks/Task.h"

ShellTaskRunner::ShellTaskRunner(QObject* parent) : QObject(parent) {}

void ShellTaskRunner::runTask(Task* task)
{
    if (!task)
        return;
    if (m_busy) {
        task->deleteLater();
        emit taskFailed(tr("Another task is running"));
        return;
    }

    task->setParent(this);
    m_task = task;
    m_busy = true;
    m_progress = -1;
    m_status.clear();
    emit busyChanged();
    emit progressChanged();
    emit statusTextChanged();

    connect(task, &Task::progress, this, [this](qint64 current, qint64 total) {
        qreal p = -1;
        if (total > 0) {
            p = qreal(current) / qreal(total);
            if (p > 1.0)
                p = 1.0;
            if (p < 0.0)
                p = 0.0;
        }
        if (p != m_progress) {
            m_progress = p;
            emit progressChanged();
        }
    });
    connect(task, &Task::status, this, [this](const QString& status) {
        if (status != m_status) {
            m_status = status;
            emit statusTextChanged();
        }
    });
    connect(task, &Task::succeeded, this, [this] { emit taskFinished(); });
    connect(task, &Task::failed, this, [this](const QString& reason) { emit taskFailed(reason); });
    connect(task, &Task::finished, this, [this, task] {
        m_task = nullptr;
        m_busy = false;
        emit busyChanged();
        task->deleteLater();
    });

    task->start();
}

void ShellTaskRunner::abort()
{
    if (m_task && m_task->canAbort())
        m_task->abort();
}
