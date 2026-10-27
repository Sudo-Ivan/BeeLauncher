#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>

class SettingsObject;

// QML-facing bridge over the global SettingsObject. Exposed as shellSettings.
class ShellSettings : public QObject {
    Q_OBJECT
   public:
    explicit ShellSettings(QObject* parent = nullptr);

    // Connects the bridge to the running Application. Tests never call this.
    void attachBackend();

    Q_INVOKABLE QVariant value(const QString& key) const;
    Q_INVOKABLE bool setValue(const QString& key, const QVariant& v);
    Q_INVOKABLE void resetValue(const QString& key);
    Q_INVOKABLE QString dataRoot() const;
    Q_INVOKABLE bool openSettingPath(const QString& key);
    Q_INVOKABLE bool openPath(const QString& absolutePath);
    Q_INVOKABLE QVariantList iconThemes() const;
    Q_INVOKABLE QVariantList appThemes() const;
    Q_INVOKABLE QString appVersion() const;
    Q_INVOKABLE QString qtVersion() const;
    Q_INVOKABLE QString gitRef() const;
    Q_INVOKABLE void useClassicUi();

   signals:
    void changed(const QString& key);

   private:
    SettingsObject* store() const;
    bool m_backend = false;
};
