#include <QTest>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QQuickItem>
#include <QStandardItemModel>
#include <QQuickImageProvider>
#include "ui/shell/ShellInstanceModel.h"
#include "InstanceList.h"
#include "ShellMocks.h"

#include <QtQml/qqmlextensionplugin.h>
Q_IMPORT_QML_PLUGIN(Bee_ShellPlugin);

// Interaction tests for the instance cards in the QML shell. The hover
// overlay buttons (play/stop, more) must actually receive clicks instead of
// being swallowed by the card-wide mouse area.
//
// Repeater delegates are not in the QObject children() tree, so lookup walks
// the visual childItems() hierarchy instead of findChildren().
class ShellCard_test : public QObject {
    Q_OBJECT
   private:
    static void walk(QQuickItem* it, const std::function<void(QQuickItem*)>& fn)
    {
        if (!it)
            return;
        fn(it);
        for (auto* ch : it->childItems())
            walk(ch, fn);
    }

    static QList<QQuickItem*> cards(QQuickWindow* win)
    {
        QList<QQuickItem*> out;
        walk(win->contentItem(), [&](QQuickItem* it) {
            if (it->objectName() == QStringLiteral("instanceCard"))
                out.append(it);
        });
        return out;
    }

    static QQuickItem* named(QQuickItem* root, const QString& objectName)
    {
        QQuickItem* found = nullptr;
        walk(root, [&](QQuickItem* it) {
            if (!found && it->objectName() == objectName)
                found = it;
        });
        return found;
    }

    static void clickCenter(QQuickWindow* win, QQuickItem* it)
    {
        const QPointF pos = it->mapToScene(QPointF(it->width() / 2, it->height() / 2));
        QTest::mouseClick(win, Qt::LeftButton, Qt::NoModifier, QPoint(pos.x(), pos.y()));
    }

   private slots:
    void initTestCase()
    {
        QQuickWindow::setSceneGraphBackend(QStringLiteral("software"));
    }

    void test_card_play_stop_and_menu()
    {
        QQmlEngine engine;
        engine.addImageProvider("beeicons", new SolidIconProvider());

        QStandardItemModel src;
        fillMockInstances(src);

        ShellInstanceModel proxy(&src);
        MockShellController controller;
        engine.rootContext()->setContextProperty("instances", &proxy);
        engine.rootContext()->setContextProperty("controller", &controller);
        registerShellContextMocks(engine.rootContext(), &controller);
        engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

        QQmlComponent comp(&engine, QUrl("qrc:/qt/qml/Bee/Shell/ui/shell/Main.qml"));
        std::unique_ptr<QObject> obj(comp.create());
        QVERIFY2(obj, qPrintable(comp.errorString()));

        auto* win = qobject_cast<QQuickWindow*>(obj.get());
        QVERIFY(win);
        win->resize(1280, 800);
        obj->setProperty("pageIndex", 1);  // library
        win->show();
        QVERIFY(QTest::qWaitForWindowExposed(win));
        QTest::qWait(300);

        const auto allCards = cards(win);
        QCOMPARE(allCards.size(), 6);
        auto* card = allCards.at(0);
        const QString id = card->property("instanceId").toString();
        QVERIFY(!id.isEmpty());

        // hidden until hovered: the overlay buttons must not react to clicks
        auto* play = named(card, "cardPlayButton");
        QVERIFY2(play, "card is missing objectName cardPlayButton");
        QCOMPARE(play->opacity(), 0.0);

        // hover the card so the overlay buttons appear
        const QPointF center = card->mapToScene(QPointF(card->width() / 2, card->height() / 2));
        QTest::mouseMove(win, QPoint(center.x(), center.y()));
        QTest::qWait(300);
        QVERIFY2(play->opacity() > 0.9, qPrintable(QString("play button opacity %1 while hovered").arg(play->opacity())));

        // play click launches the instance
        clickCenter(win, play);
        QTest::qWait(100);
        QCOMPARE(controller.launchCalls, 1);
        QCOMPARE(controller.lastLaunch, id);

        // mark the instance running: the same button must now stop it.
        // dataChanged rebuilds the repeater delegates, so re-fetch the card.
        for (int r = 0; r < src.rowCount(); ++r) {
            auto* it = src.item(r);
            it->setData(it->data(InstanceList::InstanceIDRole).toString() == id, Qt::UserRole + 23);
        }
        QTest::qWait(300);
        QQuickItem* runningCard = nullptr;
        for (auto* c : cards(win))
            if (c->property("instanceId").toString() == id)
                runningCard = c;
        QVERIFY(runningCard);
        QCOMPARE(runningCard->property("running").toBool(), true);
        QTest::mouseMove(win, QPoint(center.x(), center.y()));
        QTest::qWait(300);
        play = named(runningCard, "cardPlayButton");
        QVERIFY(play);
        QVERIFY2(play->opacity() > 0.9, "play button not visible while hovered");

        clickCenter(win, play);
        QTest::qWait(100);
        QCOMPARE(controller.killCalls, 1);
        QCOMPARE(controller.lastKill, id);

        // the more button must open the card menu
        auto* more = named(runningCard, "cardMoreButton");
        QVERIFY2(more, "card is missing objectName cardMoreButton");
        clickCenter(win, more);
        QTest::qWait(300);
        auto* menu = win->findChild<QObject*>("cardMenu");
        QVERIFY(menu);
        QVERIFY2(menu->property("opened").toBool() || menu->property("visible").toBool(), "card menu did not open");
    }
};

QTEST_MAIN(ShellCard_test)
#include "ShellCard_test.moc"
