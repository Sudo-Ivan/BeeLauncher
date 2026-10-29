#pragma once
#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "tasks/Task.h"

// QML-facing model for the official Minecraft news feed used by the shell
// News page. attachBackend() wires it to the running Application; tests never
// call it.
class ShellNewsModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
   public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        TagRole,
        CategoryRole,
        DateRole,
        TextRole,
        ImageUrlRole,
        LinkRole,
    };

    explicit ShellNewsModel(QObject* parent = nullptr);

    void attachBackend();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool busy() const { return m_busy; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void openLink(const QString& url);

   signals:
    void countChanged();
    void busyChanged();
    void lastErrorChanged();
    void fetchFailed(QString reason);

   private:
    struct Entry {
        QString title;
        QString tag;
        QString category;
        QString date;
        QString text;
        QString imageUrl;
        QString link;
    };

    void setBusy(bool busy);
    void setLastError(const QString& error);
    void parseEntries(const QByteArray& response);

    bool m_backend = false;
    bool m_busy = false;
    QString m_lastError;
    QList<Entry> m_entries;
    Task::Ptr m_job;
    int m_generation = 0;
};
