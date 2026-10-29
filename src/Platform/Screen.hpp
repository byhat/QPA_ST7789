/**
 * @file Screen.hpp
 * @brief Экран QPA: геометрия и формат.
 */

#pragma once

#include <qpa/qplatformscreen.h>

class QpaSt7789Cursor;

/**
 * @class QpaSt7789Screen
 * @brief Экран 128x160 RGB565; данные окон льются на панель напрямую
 *        из backing store, собственный композиционный буфер не нужен.
 */
class QpaSt7789Screen : public QPlatformScreen
{
public:
    /**
     * @brief Запоминает размеры экрана и создаёт программный курсор.
     * @param width ширина экрана в пикселях
     * @param height высота экрана в пикселях
     */
    QpaSt7789Screen(int width, int height);

    /**
     * @brief Геометрия экрана в пикселях.
     * @return прямоугольник (0, 0, width, height)
     */
    QRect geometry() const override;

    /**
     * @brief Глубина цвета экрана.
     * @return 16 бит (RGB565)
     */
    int depth() const override { return 16; }

    /**
     * @brief Формат экранного изображения.
     * @return QImage::Format_RGB16 (native little-endian)
     */
    QImage::Format format() const override { return QImage::Format_RGB16; }

    /**
     * @brief Физический размер панели в миллиметрах (1.14" диагональ).
     * @return размер по X и Y
     */
    QSizeF physicalSize() const override;

    /**
     * @brief Логический DPI фиксирован на 96.
     *
     * Из реального physicalSize() Qt вывел бы 113.7 DPI и включил
     * High-DPI фактор 1.1847: сцена 128 логических пикселей растянулась
     * бы в 151 физический и обрезалась по краю. Для встраиваемого
     * экрана логический пиксель должен равняться физическому.
     * @return (96, 96)
     */
    QDpi logicalDpi() const override { return QDpi(96, 96); }

    /**
     * @brief Имя экрана, видимое в Qt.
     * @return строка "ST7789V SPI"
     */
    QString name() const override { return QStringLiteral("ST7789V SPI"); }

    /**
     * @brief Программный курсор экрана.
     * @return курсор; владение за экраном
     */
    QPlatformCursor *cursor() const override;

    /**
     * @brief Доступ к курсору с типом плагина.
     * @return курсор для композиции и синхронизации позиции
     */
    QpaSt7789Cursor *st7789Cursor() const { return m_cursor; }

private:
    int m_width;                  ///< ширина экрана в пикселях
    int m_height;                 ///< высота экрана в пикселях
    QpaSt7789Cursor *m_cursor;    ///< программный курсор; владение здесь
};
