#pragma once
#include <QObject>
#include <QStandardItemModel>
#include <QQuickImageProvider>
#include <QPixmap>
#include <QColor>
#include <QHash>
#include <QPainter>
#include <QDateTime>
#include <QDir>
#include "InstanceList.h"

// Shared test/preview helpers. Not linked into the shipped app.

class SolidIconProvider : public QQuickImageProvider {
   public:
    SolidIconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override
    {
        int px = requestedSize.width() > 0 ? requestedSize.width() : 96;
        static const char* hues[] = { "#e8a33d", "#7c9f4b", "#4b8f9f", "#8f6fbf", "#bf5f5f" };
        int k = id.isEmpty() ? 0 : (id.at(0).toLatin1() % 5 + 5) % 5;
        QPixmap p(px, px);
        p.fill(Qt::transparent);
        QPainter painter(&p);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(hues[k]));
        painter.drawRoundedRect(QRectF(0, 0, px, px), px * 0.18, px * 0.18);
        painter.setPen(Qt::white);
        QFont f = painter.font();
        f.setPixelSize(px * 0.45);
        f.setBold(true);
        painter.setFont(f);
        painter.drawText(QRectF(0, 0, px, px), Qt::AlignCenter, id.isEmpty() ? "?" : QString(id.at(0).toUpper()));
        if (size) *size = p.size();
        return p;
    }
};

class MockShellController : public QObject {
    Q_OBJECT
   public:
    using QObject::QObject;
    Q_INVOKABLE void launch(const QString& id) { emit toast("launch " + id); }
    Q_INVOKABLE void kill(const QString& id) { emit toast("kill " + id); }
    Q_INVOKABLE void edit(const QString& id) { emit toast("edit " + id); }
    Q_INVOKABLE void openFolder(const QString& id) { emit toast("folder " + id); }
    Q_INVOKABLE void copyInstance(const QString& id) { emit toast("copy " + id); }
    Q_INVOKABLE void deleteInstance(const QString& id) { emit toast("delete " + id); }
    Q_INVOKABLE void createShortcut(const QString& id) { emit toast("shortcut " + id); }
    Q_INVOKABLE void changeGroup(const QString& id, const QString& g) { emit toast("group " + id); }
    Q_INVOKABLE void backupNow(const QString& id) { emit toast("backup " + id); }
    Q_INVOKABLE void repair(const QString& id) { emit toast("repair " + id); }
    Q_INVOKABLE void addInstance() { emit toast("addInstance"); }
    Q_INVOKABLE void openSettings() { emit toast("openSettings"); }
    Q_INVOKABLE void openAccounts() { emit toast("openAccounts"); }
   signals:
    void toast(const QString& message);
};

inline void fillMockInstances(QStandardItemModel& m)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    struct Row { const char* id; const char* name; const char* group; const char* ver; qint64 last; qint64 total; const char* icon; };
    const Row rows[] = {
        { "fabulously-optimized", "Fabulously Optimized", "Modded", "1.21.4 Fabric", now - 5 * 60000, 47 * 3600000LL, "fab" },
        { "create-modpack", "Create Modpack", "Modded", "1.20.1 Forge", now - 3 * 3600000LL, 12 * 3600000LL, "create" },
        { "skyfactory", "SkyFactory 4", "Modded", "1.12.2 Forge", 0, 0, "sky" },
        { "vanilla-1214", "Vanilla 1.21.4", "", "1.21.4", now - 4 * 24 * 3600000LL, 3 * 3600000LL, "vanilla" },
        { "lightly-modded", "Lightly Modded", "", "1.20.1 Fabric", 0, 900000LL, "lightly" },
        { "speedrun", "Speedrun Practice", "", "1.16.1", now - 30 * 60000, 8 * 3600000LL, "speed" },
    };
    for (auto& r : rows) {
        auto* it = new QStandardItem(r.name);
        it->setData(r.id, InstanceList::InstanceIDRole);
        it->setData(r.group, InstanceList::GroupRole);
        it->setData(r.icon, Qt::UserRole + 20);
        it->setData(r.last, Qt::UserRole + 21);
        it->setData(r.total, Qt::UserRole + 22);
        it->setData(r.ver, Qt::UserRole + 24);
        m.appendRow(it);
    }
}

class FileIconProvider : public QQuickImageProvider {
   public:
    FileIconProvider(const QString& dir) : QQuickImageProvider(QQuickImageProvider::Pixmap), m_dir(dir) {}
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override
    {
        QString path = m_dir + "/" + id + ".png";
        QPixmap p(path);
        if (p.isNull()) { p = QPixmap(requestedSize.isValid() ? requestedSize : QSize(96,96)); p.fill(QColor("#555")); }
        if (requestedSize.isValid()) p = p.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (size) *size = p.size();
        return p;
    }
   private:
    QString m_dir;
};

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
inline void fillModrinthInstances(QStandardItemModel& m, const QString& dir)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QStringList vanilla = { "Vanilla 1.21.4", "Vanilla 1.8.9" };
    for (const QString& f : QDir(dir).entryList(QStringList{ "*.json" }, QDir::Files)) {
        QFile jf(dir + "/" + f);
        if (!jf.open(QIODevice::ReadOnly)) continue;
        QJsonObject o = QJsonDocument::fromJson(jf.readAll()).object();
        auto* it = new QStandardItem(o.value("title").toString());
        it->setData(o.value("slug").toString(), InstanceList::InstanceIDRole);
        QString group = o.value("group").toString();
        it->setData(group, InstanceList::GroupRole);
        it->setData(o.value("slug").toString(), Qt::UserRole + 20);
        qint64 last = now - (qAbs(o.value("slug").toString().length() % 10) + 1) * 3600000LL * (1 + f.length() % 48);
        it->setData(last, Qt::UserRole + 21);
        it->setData(qint64((o.value("downloads").toDouble() / 1000000) * 3600000) % (60 * 3600000LL), Qt::UserRole + 22);
        it->setData(o.value("game_version").toString() + " " + o.value("loader").toString(), Qt::UserRole + 24);
        m.appendRow(it);
    }
}
