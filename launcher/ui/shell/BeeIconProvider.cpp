#include "BeeIconProvider.h"
#include <QCoreApplication>
#include <QImage>
#include <QPixmap>
#include <QPointer>
#include <QTimer>
#include "../../Application.h"
#include "../../icons/IconList.h"
#include "../../minecraft/auth/AccountList.h"
#include "../../minecraft/auth/MinecraftAccount.h"

class BeeIconResponse : public QQuickImageResponse {
   public:
    BeeIconResponse(const QString& id, const QSize& size)
    {
        const int px = size.width() > 0 ? size.width() : 128;
        // QIcon::fromTheme and QIcon::pixmap go through the shared QIconLoader,
        // which is only safe on the GUI thread, so resolve the icon there.
        QPointer<BeeIconResponse> self(this);
        QTimer::singleShot(0, qApp, [self, id, px] {
            if (!self)
                return;
            self->m_image = APPLICATION->icons()->getIcon(id).pixmap(px, px).toImage();
            emit self->finished();
        });
    }
    QQuickTextureFactory* textureFactory() const override { return QQuickTextureFactory::textureFactoryForImage(m_image); }
    QImage m_image;
};

QQuickImageResponse* BeeIconProvider::requestImageResponse(const QString& id, const QSize& requestedSize)
{
    return new BeeIconResponse(id, requestedSize);
}

class BeeAvatarResponse : public QQuickImageResponse {
   public:
    BeeAvatarResponse(const QString& id, const QSize& size)
    {
        const int px = size.width() > 0 ? size.width() : 64;
        const QString internalId = id.section('?', 0, 0);
        QPointer<BeeAvatarResponse> self(this);
        QTimer::singleShot(0, qApp, [self, internalId, px] {
            if (!self)
                return;
            auto* accounts = APPLICATION->accounts();
            for (int i = 0; i < accounts->count(); ++i) {
                auto account = accounts->at(i);
                if (account && account->internalId() == internalId) {
                    self->m_image = account->getFace(px, px).toImage();
                    break;
                }
            }
            emit self->finished();
        });
    }
    QQuickTextureFactory* textureFactory() const override { return QQuickTextureFactory::textureFactoryForImage(m_image); }
    QImage m_image;
};

QQuickImageResponse* BeeAvatarProvider::requestImageResponse(const QString& id, const QSize& requestedSize)
{
    return new BeeAvatarResponse(id, requestedSize);
}
