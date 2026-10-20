#include "BeeIconProvider.h"
#include "../../Application.h"
#include "../../icons/IconList.h"
#include <QPixmap>
#include <QtConcurrent/QtConcurrent>

class BeeIconResponse : public QQuickImageResponse {
   public:
    BeeIconResponse(const QString& id, const QSize& size)
    {
        m_future = QtConcurrent::run([id, size]() -> QPixmap {
            int px = size.width() > 0 ? size.width() : 128;
            return APPLICATION->icons()->getIcon(id).pixmap(px, px);
        });
        m_watcher.setFuture(m_future);
        connect(&m_watcher, &QFutureWatcher<QPixmap>::finished, this, &QQuickImageResponse::finished);
    }
    QQuickTextureFactory* textureFactory() const override { return QQuickTextureFactory::textureFactoryForImage(m_watcher.result().toImage()); }
    QFuture<QPixmap> m_future;
    QFutureWatcher<QPixmap> m_watcher;
};

QQuickImageResponse* BeeIconProvider::requestImageResponse(const QString& id, const QSize& requestedSize)
{
    return new BeeIconResponse(id, requestedSize);
}
