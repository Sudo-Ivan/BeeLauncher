#pragma once
#include <QQuickImageProvider>

class BeeIconProvider : public QQuickAsyncImageProvider {
   public:
    QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) override;
};

// Resolves account internal ids to Minecraft skin faces for QML. Returns an
// empty image for accounts without a skin.
class BeeAvatarProvider : public QQuickAsyncImageProvider {
   public:
    QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) override;
};
