/**
 * @file St7789Driver.hpp
 * @brief Драйвер SPI-панели ST7789 поверх шины spidev и sysfs-GPIO.
 */

#pragma once

#include "SpidevBus.hpp"
#include "WiringGpioLine.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * @class St7789Driver
 * @brief Управляет панелью ST7789 через шину и GPIO-линии.
 *
 * Зависимости прокидываются извне по ссылке и должны переживать драйвер.
 *
 * Формат данных blit(): пиксели RGB565 в нативном порядке uint16_t
 * (little-endian, как QImage::Format_RGB16) с байтовым stride; драйвер
 * сам меняет байты местами под big-endian шину - тот же контракт, что у
 * QPA-плагина, для будущей интеграции.
 *
 * Особенности ST7789 и панелей 1.14" 135x240 (учтены внутри):
 * - контроллер имеет RAM 240x320, видимая область 135x240 смещена;
 *   сдвиг окна задаётся offsetX/offsetY (обычно 40/52, у части панелей
 *   52/40 или 53/40 - подбирается по картинке);
 * - одиночные команды панель принимает только на низкой скорости,
 *   поэтому инициализация идёт на initSpeedHz;
 * - у панелей 1.14" цвета инвертированы от рождения: при
 *   invertColors в инициализацию добавляется команда INVON;
 * - уровень DC выставляется перед каждой строкой кадра: pwrite в sysfs
 *   между близкими передачами держит уровень стабильным, без этого
 *   панель срывает поток данных после первого кадра.
 */
class St7789Driver
{
public:
    /**
     * @struct Config
     * @brief Параметры панели и скоростей шины.
     */
    struct Config
    {
        int width;                 ///< ширина панели, пикселей
        int height;                ///< высота панели, пикселей
        std::uint32_t initSpeedHz; ///< скорость одиночных команд инициализации
        std::uint32_t dataSpeedHz; ///< скорость потока пикселей
        int offsetX;               ///< сдвиг столбцов RAM (1.14": обычно 40)
        int offsetY;               ///< сдвиг строк RAM (1.14": обычно 52)
        bool invertColors;         ///< инверсия цветов (INVON, нужна 1.14")
    };

    /**
     * @brief Хранит ссылки на шину и линии, ресурсы не создаёт.
     * @param bus SPI-шина для передач
     * @param chipSelect линия CS панели (активный низкий)
     * @param dataCommand линия DC панели (низкий = команда)
     * @param reset линия аппаратного сброса панели
     * @param config параметры панели и скоростей
     */
    St7789Driver(SpidevBus &bus, WiringGpioLine &chipSelect,
                 WiringGpioLine &dataCommand, WiringGpioLine &reset,
                 const Config &config);

    /**
     * @brief Инициализирует панель: CS вниз, аппаратный и программный
     *        сброс, выход из сна, формат пикселя, инверсия, включение
     *        отображения.
     *
     * Вызывается один раз после конструирования, до fill()/blit().
     */
    void initialize();

    /**
     * @brief Заливает весь экран одним цветом.
     * @param rgb565 цвет в формате RGB565 (native uint16_t)
     */
    void fill(std::uint16_t rgb565);

    /**
     * @brief Копирует прямоугольник пикселей из внешнего буфера на экран.
     * @param x левый край области, пикселей от нуля
     * @param y верхний край области, пикселей от нуля
     * @param width ширина области, пикселей
     * @param height высота области, пикселей
     * @param pixels буфер кадра RGB565 (native uint16_t)
     * @param strideBytes шаг строки буфера в байтах
     */
    void blit(int x, int y, int width, int height,
              const std::uint16_t *pixels, std::size_t strideBytes);

private:
    /**
     * @brief Отправляет одну команду контроллеру (DC = низкий).
     * @param command байт команды
     */
    void writeCommand(std::uint8_t command);

    /**
     * @brief Отправляет данные контроллеру (DC = высокий).
     * @param data буфер данных
     * @param size размер буфера в байтах
     */
    void writeData(const std::uint8_t *data, std::size_t size);

    /**
     * @brief Задаёт RAM-окно панели командами CASET/RASET с учётом
     *        сдвига offsetX/offsetY и включает запись RAMWR.
     * @param x0 начальный столбец
     * @param y0 начальная строка
     * @param x1 конечный столбец включительно
     * @param y1 конечная строка включительно
     */
    void setWindow(int x0, int y0, int x1, int y1);

    /**
     * @brief Аппаратный сброс панели импульсом на линии RST
     *        с паузами из даташита ST7789.
     */
    void resetHardware();

    /// Команды ST7789, используемые драйвером.
    enum class Command : std::uint8_t
    {
        SwReset = 0x01, ///< программный сброс
        SlpOut = 0x11,  ///< выход из сна
        NorOn = 0x13,   ///< нормальный режим отображения
        DisOn = 0x29,   ///< включение отображения
        InvOn = 0x21,   ///< инверсия цветов
        Caset = 0x2A,   ///< диапазон столбцов RAM
        Raset = 0x2B,   ///< диапазон строк RAM
        RamWr = 0x2C,   ///< запись в RAM
        Madctl = 0x36,  ///< ориентация/зеркалирование
        Colmod = 0x3A,  ///< формат пикселя
    };

    /// Значение COLMOD: 16 бит на пиксель (RGB565) у ST7789.
    static constexpr std::uint8_t kPixelFormatRgb565 = 0x55;

    /// Значение MADCTL: без поворота и зеркалирования.
    static constexpr std::uint8_t kMemoryDataAccessControlDefault = 0x00;

    SpidevBus &bus_;              ///< шина передач (не владеется)
    WiringGpioLine &chipSelect_;   ///< линия CS (не владеется)
    WiringGpioLine &dataCommand_;  ///< линия DC (не владеется)
    WiringGpioLine &reset_;        ///< линия RST (не владеется)
    Config config_;               ///< параметры панели и скоростей
    std::vector<std::uint8_t> lineBuffer_; ///< строка пикселей в big-endian
};
