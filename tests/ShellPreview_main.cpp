// Dev-only preview for the QML shell. Never installed, never shipped.
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QCommandLineParser>
#include <QTimer>
#include <QtTest/QTest>
#include <QElapsedTimer>
#include "ShellMocks.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QtQml/qqmlextensionplugin.h>
#include "ui/shell/ShellInstanceModel.h"

Q_IMPORT_QML_PLUGIN(Bee_ShellPlugin);

int main(int argc, char** argv)
{
    QQuickWindow::setSceneGraphBackend(QStringLiteral("software"));
    QGuiApplication app(argc, argv);
    QCommandLineParser p;
    p.addOptions({ { "page", "page", "home" }, { "theme", "theme", "dark" }, { "width", "w", "1280" },
                   { "height", "h", "800" }, { "output", "png", "/tmp/preview.png" }, { "select", "instance index to select", "-1" },
                   { "detail", "open instance detail overlay", "" }, { "empty", "empty model", "0" }, { "hover", "card index to hover", "-1" }, { "menu", "open card context menu", "" }, { "assets", "modrinth asset dir", "dir" } });
    p.process(app);

    QQmlEngine engine;
    if (p.isSet("assets"))
        engine.addImageProvider("beeicons", new FileIconProvider(p.value("assets")));
    else
        engine.addImageProvider("beeicons", new SolidIconProvider());
    QStandardItemModel src;
    if (!p.isSet("empty") || p.value("empty") == "0") {
        if (p.isSet("assets"))
            fillModrinthInstances(src, p.value("assets"));
        else
            fillMockInstances(src);
    }
    fprintf(stderr, "rows=%d assets=%s\n", src.rowCount(), qPrintable(p.value("assets")));
    ShellInstanceModel proxy(&src);
    MockShellController controller;
    engine.rootContext()->setContextProperty("instances", &proxy);
    engine.rootContext()->setContextProperty("controller", &controller);
    registerShellContextMocks(engine.rootContext(), &controller);
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

    QQmlComponent comp(&engine, QUrl("qrc:/qt/qml/Bee/Shell/ui/shell/Main.qml"));
    std::unique_ptr<QObject> obj(comp.create());
    if (!obj) {
        qWarning() << comp.errorString();
        return 2;
    }
    auto* win = qobject_cast<QQuickWindow*>(obj.get());
    if (!win) return 3;
    win->resize(p.value("width").toInt(), p.value("height").toInt());
    win->show();

    // page select + theme + selection via root properties
    QMetaObject::invokeMethod(obj.get(), [&]() {
        static const QHash<QString, int> pages = { { "home", 0 }, { "library", 1 }, { "servers", 2 }, { "discover", 3 }, { "accounts", 4 }, { "settings", 5 } };
        if (p.isSet("page") && pages.contains(p.value("page")))
            obj->setProperty("pageIndex", pages.value(p.value("page")));
        obj->setProperty("lightMode", p.value("theme") == "light");
        if (p.isSet("detail"))
            obj->setProperty("detailId", p.value("detail"));
        int sel = p.isSet("select") ? p.value("select").toInt() : -1;
        if (sel >= 0)
            QMetaObject::invokeMethod(obj.get(), "pickInstance", Q_ARG(QVariant, sel));
    }, Qt::QueuedConnection);

    QObject::connect(win, &QQuickWindow::frameSwapped, win, [&]() {
        fprintf(stderr, "graphicsApi=%d\n", int(win->graphicsApi()));
        QTimer::singleShot(300, [&]() {
            int hov = p.value("hover").toInt();
            if (hov >= 0 || p.isSet("menu")) {
                auto cards = win->contentItem()->findChildren<QObject*>("instanceCard");
                if (hov < 0) hov = 0;
                if (hov < cards.size()) {
                    auto* card = qobject_cast<QQuickItem*>(cards.at(hov));
                    QPointF c = card->mapToScene(QPointF(card->width() / 2, card->height() / 2));
                    QTest::mouseMove(win, QPoint(c.x(), c.y()));
                    if (p.isSet("menu"))
                        QMetaObject::invokeMethod(card, "menuRequested", Q_ARG(QVariant, card->width() - 10), Q_ARG(QVariant, 10.0));
                }
            }
            QTimer::singleShot(350, [&]() {
                win->grabWindow().save(p.value("output"));
                // dev tool: skip static teardown to avoid plugin shutdown crash
                std::fflush(nullptr);
                std::_Exit(0);
            });
        });
    });
    QTimer::singleShot(3000, &app, &QGuiApplication::quit);  // hard cap: never leave windows open
    return app.exec();
}
