#include <QTest>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QQuickItem>
#include <QStandardItemModel>
#include <QQuickImageProvider>
#include <QQuickTextureFactory>
#include <QPixmap>
#include <QPainter>
#include "ui/shell/ShellController.h"
#include "ui/shell/ShellInstanceModel.h"
#include "InstanceList.h"
#include "ShellMocks.h"

#include <QtQml/qqmlextensionplugin.h>
Q_IMPORT_QML_PLUGIN(Bee_ShellPlugin);

class ShellRender_test : public QObject {
    Q_OBJECT
   private slots:
    void initTestCase()
    {
        QQuickWindow::setSceneGraphBackend(QStringLiteral("software"));
    }

    void test_render_library_and_empty()
    {
        QQmlEngine engine;
        engine.addImageProvider("beeicons", new SolidIconProvider());

        QStandardItemModel src;
        auto addInst = [&](const QString& id, const QString& name, const QString& group) {
            auto* it = new QStandardItem(name);
            it->setData(id, InstanceList::InstanceIDRole);
            it->setData(group, InstanceList::GroupRole);
            src.appendRow(it);
        };
        addInst("a", "Alpha One", "Modded");
        addInst("b", "Beta Two", "Modded");
        addInst("c", "Gamma Three", "Vanilla");

        ShellInstanceModel proxy(&src);
        ShellController controller;
        engine.rootContext()->setContextProperty("instances", &proxy);
        engine.rootContext()->setContextProperty("controller", &controller);
        registerShellContextMocks(engine.rootContext(), &controller);
        engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

        QQmlComponent comp(&engine, QUrl("qrc:/qt/qml/Bee/Shell/ui/shell/Main.qml"));
        std::unique_ptr<QObject> obj(comp.create());
        QVERIFY2(obj, qPrintable(comp.errorString()));

        auto* win = qobject_cast<QQuickWindow*>(obj.get());
        if (!win) {
            auto* item = qobject_cast<QQuickItem*>(obj.get());
            QVERIFY(item);
            QSKIP("root item is not a window in this configuration");
        }
        win->resize(1280, 800);
        win->show();
        QVERIFY(QTest::qWaitForWindowExposed(win));

        QImage img = win->grabWindow();
        if (img.isNull())
            QSKIP("grabWindow not supported in this environment");
        QVERIFY(img.width() > 0 && img.height() > 0);
        // not a uniform blank frame: at least two distinct colors present
        int colors = 0;
        QRgb first = img.pixel(4, 4);
        for (int y = 0; y < img.height(); y += 40)
            for (int x = 0; x < img.width(); x += 40)
                if (img.pixel(x, y) != first) { colors = 1; goto done; }
    done:
        QCOMPARE(colors, 1);

        QString dir = qEnvironmentVariable("BEE_SHELL_SHOTS_DIR");
        if (!dir.isEmpty()) {
            QDir().mkpath(dir);
            img.save(dir + "/library-dark.png");
            static const char* names[] = { "home", "library", "servers", "discover", "news", "accounts", "settings" };
            for (int i = 0; i < 7; ++i) {
                obj->setProperty("pageIndex", i);
                QTest::qWait(120);
                win->grabWindow().save(dir + "/" + names[i] + ".png");
            }
        }

        // empty state
        src.removeRows(0, src.rowCount());
        QTest::qWait(200);
        QImage img2 = win->grabWindow();
        QVERIFY(!img2.isNull());
        if (!dir.isEmpty())
            img2.save(dir + "/empty-dark.png");
    }
};

QTEST_MAIN(ShellRender_test)
#include "ShellRender_test.moc"
