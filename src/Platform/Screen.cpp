/**
 * @file Screen.cpp
 * @brief Реализация QpaSt7789Screen.
 */

#include "Screen.hpp"
#include "Cursor.hpp"

QpaSt7789Screen::QpaSt7789Screen(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_cursor(new QpaSt7789Cursor(QSize(width, height)))
{
}

QRect QpaSt7789Screen::geometry() const
{
    return QRect(0, 0, m_width, m_height);
}

QSizeF QpaSt7789Screen::physicalSize() const
{
    if (m_width > m_height) {
        return QSizeF(25.2, 14.2);
    }
    return QSizeF(14.2, 25.2);
}

QPlatformCursor *QpaSt7789Screen::cursor() const
{
    return m_cursor;
}
