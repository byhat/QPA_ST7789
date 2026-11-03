/**
 * @file Integration.hpp
 * @brief Платформенная интеграция QPA для SPI-экрана ST7789.
 */

#pragma once

#include "Config.hpp"

#include <qpa/qplatformintegration.h>

#include <memory>

class FramePresenter;
class KeyboardDevice;
class MouseDevice;
class QpaSt7789BackingStore;
class QpaSt7789Screen;
class QPlatformFontDatabase;
class SpidevBus;
class GpioLine;
class St7789Driver;

/**
 * @class QpaSt7789Integration
 * @brief Собирает платформу: HAL из lib, драйвер панели, экран, окна,
 *        backing store, шрифты, мышь, event loop.
 *
 * Если инициализация дисплея не удалась (нет spidev/GPIO/прав), экран
 * всё равно создаётся, а вывод игнорируется - приложение остаётся живым.
 */
class QpaSt7789Integration : public QPlatformIntegration
{
public:
    /**
     * @brief Загружает конфигурацию, инициализирует дисплей, регистрирует экран.
     */
    QpaSt7789Integration();

    /**
     * @brief Снимает экран с регистрации и освобождает ресурсы.
     */
    ~QpaSt7789Integration() override;

    /**
     * @brief Сообщает Qt о возможностях платформы.
     * @param cap проверяемая возможность
     * @return true только для MultipleWindows
     */
    bool hasCapability(Capability cap) const override;

    /**
     * @brief Создаёт платформенное окно, растянутое на весь экран.
     * @param window объект QWindow верхнего уровня
     * @return новое окно; владение переходит Qt
     */
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;

    /**
     * @brief Создаёт растровый backing store для окна.
     * @param window объект QWindow верхнего уровня
     * @return новый backing store; владение переходит Qt
     */
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

    /**
     * @brief Создаёт unix event dispatcher.
     * @return новый диспетчер; владение переходит Qt
     */
    QAbstractEventDispatcher *createEventDispatcher() const override;

    /**
     * @brief Возвращает базу шрифтов согласно fonts.dir из конфигурации.
     * @return база шрифтов, живёт столько же, сколько интеграция
     */
    QPlatformFontDatabase *fontDatabase() const override;

    /**
     * @brief Предлагает стандартную unix-тему Qt.
     * @return список из одной темы "generic"
     */
    QStringList themeNames() const override;

private:
    QpaConfig m_config;                            ///< конфигурация плагина
    std::unique_ptr<SpidevBus> m_bus;              ///< SPI-шина
    std::unique_ptr<GpioLine> m_chipSelect;   ///< линия CS
    std::unique_ptr<GpioLine> m_dataCommand;  ///< линия DC
    std::unique_ptr<GpioLine> m_reset;        ///< линия RST
    std::unique_ptr<St7789Driver> m_driver;        ///< драйвер панели
    // m_presenter объявлен после m_driver и разрушается раньше него:
    // поток отправки кадров обязан остановиться до освобождения шины
    std::unique_ptr<FramePresenter> m_presenter;   ///< поток отправки кадров
    mutable std::unique_ptr<QPlatformFontDatabase> m_fontDatabase; ///< создаётся лениво
    QpaSt7789Screen *m_screen = nullptr;           ///< экран; владение передано Qt
    mutable std::unique_ptr<MouseDevice> m_mouseDevice; ///< мышь evdev; создаётся при первом окне
    mutable std::unique_ptr<KeyboardDevice> m_keyboardDevice; ///< клавиатура evdev; создаётся при первом окне
    mutable QpaSt7789BackingStore *m_activeBackingStore = nullptr; ///< последнее созданное; владение у Qt
};
