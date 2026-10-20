#include "ShellWindow.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

#include "../../Application.h"
#include "../../InstanceList.h"
#include "../../StartupProfile.h"
#include "BeeIconProvider.h"
#include "ShellController.h"
#include "ShellInstanceModel.h"

ShellWindow::ShellWindow(QObject* parent) : QObject(parent), m_engine(new QQmlApplicationEngine)
{
    m_engine->addImportPath(QStringLiteral("qrc:/qt/qml"));
    m_engine->addImageProvider(QStringLiteral("beeicons"), new BeeIconProvider());

    m_model = new ShellInstanceModel(APPLICATION->instances(), this);
    m_controller = new ShellController(this);
    m_controller->attachBackend();
    m_engine->rootContext()->setContextProperty(QStringLiteral("instances"), m_model);
    m_engine->rootContext()->setContextProperty(QStringLiteral("controller"), m_controller);

    m_engine->load(QUrl(QStringLiteral("qrc:/qt/qml/Bee/Shell/ui/shell/Main.qml")));
    if (!m_engine->rootObjects().isEmpty())
        m_window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    StartupProfile::mark("shell window created");
    if (!m_window)
        return;

    restoreGeometry();
    connect(m_window, &QQuickWindow::closing, this, [this] {
        saveGeometry();
        emit isClosing();
    });
    connect(
        m_window, &QQuickWindow::frameSwapped, this,
        [this] {
            static bool first = true;
            if (!first)
                return;
            first = false;
            StartupProfile::mark("first frame");
            const QString grab = qEnvironmentVariable("BEE_SHELL_GRAB");
            if (!grab.isEmpty()) {
                QTimer::singleShot(500, this, [this, grab] {
                    m_window->grabWindow().save(grab);
                    QCoreApplication::exit(0);
                });
            }
        },
        Qt::QueuedConnection);
}

ShellWindow::~ShellWindow() = default;

void ShellWindow::show(bool minimized)
{
    if (!m_window)
        return;
    if (minimized)
        m_window->showMinimized();
    else
        m_window->show();
}

void ShellWindow::raiseWindow()
{
    if (!m_window)
        return;
    m_window->setWindowStates(m_window->windowStates() & ~Qt::WindowMinimized);
    m_window->show();
    m_window->raise();
    m_window->requestActivate();
}

void ShellWindow::saveGeometry()
{
    const QRect g = m_window->geometry();
    APPLICATION->settings()->set("ShellWindowGeometry", QStringLiteral("%1,%2,%3,%4").arg(g.x()).arg(g.y()).arg(g.width()).arg(g.height()));
}

void ShellWindow::restoreGeometry()
{
    const auto parts = APPLICATION->settings()->get("ShellWindowGeometry").toString().split(',');
    if (parts.size() != 4)
        return;
    const QRect g(parts[0].toInt(), parts[1].toInt(), parts[2].toInt(), parts[3].toInt());
    if (g.width() < 640 || g.height() < 400)
        return;
    m_window->resize(g.size());
    for (auto* screen : QGuiApplication::screens())
        if (screen->availableGeometry().intersects(g)) {
            m_window->setPosition(g.topLeft());
            break;
        }
}
