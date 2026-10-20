#pragma once
#include <QQuickImageProvider>

class BeeIconProvider : public QQuickAsyncImageProvider {
   public:
    QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) override;
};
