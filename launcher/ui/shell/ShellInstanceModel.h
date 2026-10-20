#pragma once
#include <QSortFilterProxyModel>

// Proxy over InstanceList (or any injected model with matching roles) for the QML shell.
class ShellInstanceModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(int sortMode READ sortModeInt WRITE setSortModeInt NOTIFY sortModeChanged)
    Q_PROPERTY(QString groupFilter READ groupFilter WRITE setGroupFilter NOTIFY groupFilterChanged)
    Q_PROPERTY(QStringList groups READ groups NOTIFY groupsChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
   public:
    enum SortMode { Name = 0, LastPlayed, PlayTime };
    enum Roles {
        InstanceIdRole = Qt::UserRole + 1,
        NameRole,
        GroupRole2,
        IconSourceRole,
        VersionTextRole,
        LastPlayedMsRole,
        TotalPlayTimeMsRole,
        RunningRole,
        StatusTextRole,
        InstancePointerShellRole,
    };

    explicit ShellInstanceModel(QObject* parent = nullptr) : ShellInstanceModel(nullptr, parent) {}
    explicit ShellInstanceModel(QAbstractItemModel* source, QObject* parent = nullptr);
    QString filterText() const { return m_filter; }
    void setFilterText(const QString& t);
    SortMode sortMode() const { return m_sortMode; }
    void setSortMode(SortMode m);
    int sortModeInt() const { return static_cast<int>(m_sortMode); }
    void setSortModeInt(int m) { setSortMode(static_cast<SortMode>(m)); }
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Q_INVOKABLE QString instanceIdAt(int row) const;
    Q_INVOKABLE QString groupAt(int row) const { return data(index(row, 0), GroupRole2).toString(); }
    QString groupFilter() const { return m_groupFilter; }
    void setGroupFilter(const QString& g);
    QStringList groups() const;
   signals:
    void filterTextChanged();
    void sortModeChanged();
    void groupFilterChanged();
    void groupsChanged();
    void countChanged();
   protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override;
    bool lessThan(const QModelIndex& l, const QModelIndex& r) const override;
   private slots:
    void refreshGroups();
   private:
    QString m_filter;
    QString m_groupFilter;
    SortMode m_sortMode = Name;
};
