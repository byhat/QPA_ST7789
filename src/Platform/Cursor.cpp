/**
 * @file Cursor.cpp
 * @brief Реализация QpaSt7789Cursor: позиция, клэмп, отрисовка стрелки.
 */

#include "Cursor.hpp"

#include <QPainter>

namespace {

/**
 * @brief Стрелка-курсор 12x15: треугольник, наклонённый влево.
 *
 * Классический силуэт без "хвоста": вертикальная левая грань,
 * диагональ справа, скошенное основание. Кончик - верхний левый
 * пиксель (hotspot 0,0). Пробел - прозрачность.
 */
const char *const kArrowXpm[] = {
    "12 15 3 1",
    "  c None",
    "# c #000000",
    ". c #ffffff",
    "#           ",
    "##          ",
    "#.#         ",
    "#..#        ",
    "#...#       ",
    "#....#      ",
    "#.....#     ",
    "#......#    ",
    "#.......#   ",
    "#........#  ",
    "#.........# ",
    "#.......#   ",
    "#....#      ",
    "#..#        ",
    "#           ",
};

} // namespace

QpaSt7789Cursor::QpaSt7789Cursor(const QSize &screenSize)
    : m_screenSize(screenSize)
{
}

void QpaSt7789Cursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    Q_UNUSED(window);
    if (windowCursor == nullptr) {
        return;
    }
    m_shape = windowCursor->shape();
}

QPoint QpaSt7789Cursor::pos() const
{
    return m_position;
}

void QpaSt7789Cursor::setPos(const QPoint &pos)
{
    m_position = QPoint(qBound(0, pos.x(), m_screenSize.width() - 1),
                        qBound(0, pos.y(), m_screenSize.height() - 1));
    m_revealed = true;
}

void QpaSt7789Cursor::moveBy(int dx, int dy)
{
    setPos(m_position + QPoint(dx, dy));
}

void QpaSt7789Cursor::setShowCursor(bool show)
{
    m_showCursor = show;
}

void QpaSt7789Cursor::paint(QPainter *painter) const
{
    if (!shouldDraw()) {
        return;
    }
    painter->drawPixmap(m_position, arrowPixmap());
}

bool QpaSt7789Cursor::shouldDraw() const
{
    return m_showCursor && m_revealed && m_shape != Qt::BlankCursor;
}

const QPixmap &QpaSt7789Cursor::arrowPixmap() const
{
    if (m_arrow.isNull()) {
        m_arrow = QPixmap(kArrowXpm);
    }
    return m_arrow;
}
