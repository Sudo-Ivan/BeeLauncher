#include <QTest>
#include <QImage>
#include <QPixmap>
#include "minecraft/auth/MinecraftAccount.h"

// An account without a custom skin must still produce a face (the default
// Steve head) so avatars are never blank.
class AccountFace_test : public QObject {
    Q_OBJECT
   private slots:
    void initTestCase()
    {
        // The steve icon lives in a qrc embedded in Launcher_logic; make sure
        // it is registered when the test links the static lib.
        Q_INIT_RESOURCE(multimc);
    }

    void test_default_face_when_no_skin()
    {
        auto account = MinecraftAccount::createOffline(QStringLiteral("Steveless"));
        QVERIFY(account);
        QVERIFY(account->accountData()->minecraftProfile.skin.data.isEmpty());

        QPixmap face = account->getFace();
        QVERIFY2(!face.isNull(), "face must fall back to the default Steve head");
        QCOMPARE(face.size(), QSize(64, 64));

        // and it must not be fully transparent
        const QImage img = face.toImage();
        bool hasOpaque = false;
        for (int y = 0; y < img.height() && !hasOpaque; ++y)
            for (int x = 0; x < img.width(); ++x)
                if (qAlpha(img.pixel(x, y)) > 0) {
                    hasOpaque = true;
                    break;
                }
        QVERIFY(hasOpaque);
    }

    void test_default_face_scales()
    {
        auto account = MinecraftAccount::createOffline(QStringLiteral("Steveless"));
        QPixmap face = account->getFace(24, 24);
        QVERIFY(!face.isNull());
        QCOMPARE(face.size(), QSize(24, 24));
    }
};

QTEST_MAIN(AccountFace_test)
#include "AccountFace_test.moc"
