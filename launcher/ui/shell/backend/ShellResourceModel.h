#pragma once
#include <QSortFilterProxyModel>

class ResourceFolderModel;
class WorldList;

// Flat QML-facing proxy over a ResourceFolderModel (mods, packs) or a
// WorldList. Exposes simple named roles and row actions so the shell pages
// never have to know about the legacy column scheme.
class ShellResourceModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool canToggle READ canToggle NOTIFY sourceChanged)
    Q_PROPERTY(bool canDelete READ canDelete NOTIFY sourceChanged)
    Q_PROPERTY(QString folderPath READ folderPath NOTIFY sourceChanged)
   public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        FileNameRole,
        EnabledRole,
        SizeBytesRole,
        SizeTextRole,
        LastModifiedRole,  // ms since epoch
        InfoRole,          // mod version, provider or extra info
        GameModeRole,      // worlds only
        SeedRole,          // worlds only
    };

    explicit ShellResourceModel(QObject* parent = nullptr);
    ~ShellResourceModel() override;

    // Attaches the source model and starts its filesystem watcher.
    // Passing nullptr detaches it again.
    void setSource(QAbstractItemModel* source);

    bool canToggle() const;
    bool canDelete() const;
    QString folderPath() const;

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex& index, int role) const override;

    Q_INVOKABLE void setEnabled(int row, bool on);
    Q_INVOKABLE bool remove(int row);
    Q_INVOKABLE void openFolder();
    Q_INVOKABLE void refresh();

   signals:
    void sourceChanged();
    void countChanged();

   private:
    enum class Kind { None, Resources, Worlds };
    Kind m_kind = Kind::None;
    ResourceFolderModel* m_resources = nullptr;
    WorldList* m_worlds = nullptr;
    // Only true when our own startWatching() call succeeded, so we do not
    // stop a watcher a classic editor page still relies on.
    bool m_weWatch = false;
};
