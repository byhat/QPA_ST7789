/**
 * @file Integration.cpp
 * @brief Реализация QpaSt7789Integration.
 */

#include "Integration.hpp"
#include "BackingStore.hpp"
#include "Cursor.hpp"
#include "FramePresenter.hpp"
#include "KeyboardDevice.hpp"
#include "MouseDevice.hpp"
#include "Screen.hpp"
#include "Window.hpp"
#include "SpidevBus.hpp"
#include "St7789Driver.hpp"
#include "SysfsGpioLine.hpp"

#include <QTimer>

#include <QtGui/private/qgenericunixeventdispatcher_p.h>
#include <QtGui/private/qgenericunixfontdatabase_p.h>
#include <QtGui/private/qfreetypefontdatabase_p.h>
#include <qpa/qwindowsysteminterface.h>

QpaSt7789Integration::QpaSt7789Integration()
    : m_config(loadQpaConfig())
{
    try {
        m_bus = std::make_unique<SpidevBus>(m_config.spiDevice.toStdString());
        m_chipSelect = std::make_unique<SysfsGpioLine>(m_config.chipSelectPin);
        m_dataCommand = std::make_unique<SysfsGpioLine>(m_config.dataCommandPin);
        m_reset = std::make_unique<SysfsGpioLine>(m_config.resetPin);
        m_driver = std::make_unique<St7789Driver>(
            *m_bus, *m_chipSelect, *m_dataCommand, *m_reset, m_config.panel);
        m_driver->initialize();
        m_presenter = std::make_unique<FramePresenter>(*m_driver);
    } catch (const std::exception &error) {
        qWarning("qpast7789: init failed: %s", error.what());
    }

    m_screen = new QpaSt7789Screen(m_config.panel.width, m_config.panel.height);
    QWindowSystemInterface::handleScreenAdded(m_screen);
    m_screen->st7789Cursor()->setShowCursor(m_config.mouseShowCursor);
}

QpaSt7789Integration::~QpaSt7789Integration()
{
    QWindowSystemInterface::handleScreenRemoved(m_screen);
}

bool QpaSt7789Integration::hasCapability(Capability cap) const
{
    if (cap == MultipleWindows) {
        return true;
    }
    return QPlatformIntegration::hasCapability(cap);
}

QPlatformWindow *QpaSt7789Integration::createPlatformWindow(QWindow *window) const
{
    if (m_mouseDevice == nullptr && m_config.mouseEnabled) {
        // создаём здесь, а не в конструкторе: интеграция строится до
        // event dispatcher, и notifier/timer, созданные раньше него,
        // не регистрируются и никогда не срабатывают
        m_mouseDevice = std::make_unique<MouseDevice>(m_config.mouseDevice,
                                                      m_screen->st7789Cursor());
        // события мыши обрабатываются сразу, а публикация кадров с
        // курсором коалесцируется: не чаще раза в 33 мс, иначе полный
        // кадр (~10 мс SPI) на каждый пачку событий подвешивает ввод
        QTimer *cursorRefresh = new QTimer(m_mouseDevice.get());
        cursorRefresh->setSingleShot(true);
        cursorRefresh->setInterval(33);
        QObject::connect(cursorRefresh, &QTimer::timeout, m_mouseDevice.get(), [this]() {
            if (m_activeBackingStore != nullptr) {
                m_activeBackingStore->presentFrame();
            }
        });
        QObject::connect(m_mouseDevice.get(), &MouseDevice::cursorMoved,
                         m_mouseDevice.get(), [cursorRefresh]() {
            if (!cursorRefresh->isActive()) {
                cursorRefresh->start();
            }
        });
    }
    if (m_keyboardDevice == nullptr && m_config.keyboardEnabled) {
        m_keyboardDevice = std::make_unique<KeyboardDevice>(m_config.keyboardDevice);
    }
    return new QpaSt7789Window(window);
}

QPlatformBackingStore *QpaSt7789Integration::createPlatformBackingStore(QWindow *window) const
{
    m_activeBackingStore = new QpaSt7789BackingStore(
        window, m_presenter.get(), m_screen->st7789Cursor());
    return m_activeBackingStore;
}

QAbstractEventDispatcher *QpaSt7789Integration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformFontDatabase *QpaSt7789Integration::fontDatabase() const
{
    if (!m_fontDatabase) {
        const QString fontDir = m_config.fontDir;
        if (!fontDir.isEmpty()) {
            qputenv("QT_QPA_FONTDIR", fontDir.toLocal8Bit());
        }
#if QT_CONFIG(fontconfig)
        if (fontDir.isEmpty()) {
            m_fontDatabase.reset(new QGenericUnixFontDatabase);
        } else {
            m_fontDatabase.reset(new QFreeTypeFontDatabase);
        }
#else
        m_fontDatabase.reset(new QFreeTypeFontDatabase);
#endif
    }
    return m_fontDatabase.get();
}

QStringList QpaSt7789Integration::themeNames() const
{
    return { QStringLiteral("generic") };
}
