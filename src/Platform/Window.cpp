/**
 * @file Window.cpp
 * @brief Реализация QpaSt7789Window.
 */

#include "Window.hpp"
#include "Screen.hpp"

#include <qpa/qwindowsysteminterface.h>

QpaSt7789Window::QpaSt7789Window(QWindow *window)
    : QPlatformWindow(window)
{
    QPlatformWindow::initialize();
    setGeometry(screen()->geometry());
}

void QpaSt7789Window::setGeometry(const QRect &rect)
{
    Q_UNUSED(rect);
    const QRect fullscreen = screen()->geometry();
    QPlatformWindow::setGeometry(fullscreen);
    QWindowSystemInterface::handleGeometryChange(window(), fullscreen);
    if (window()->isVisible()) {
        QWindowSystemInterface::handleExposeEvent(window(), fullscreen);
    }
}

void QpaSt7789Window::setVisible(bool visible)
{
    if (visible) {
        requestActivateWindow();
        QWindowSystemInterface::handleExposeEvent(window(), screen()->geometry());
    } else {
        QWindowSystemInterface::handleExposeEvent(window(), QRegion());
    }
}

void QpaSt7789Window::requestUpdate()
{
    deliverUpdateRequest();
}
