#include "Point.h"
#include "cmath"

/*
 * 获取和其他节点的直线距离
 */
double Point::getDistance(Point *other)
{
    return pow(pow(other->x - this->x, 2) + pow(other->y - this->y, 2), 0.5);
}

/**
 * 判断节点是否被选中，即判断点击坐标是否在节点范围内
*/
bool Point::selected(int mouse_x, int mouse_y)
{
    int minX = this->x * 5 - (int)(Point::size / 2);
    int minY = this->y * 5 - (int)(Point::size / 2);
    int maxX = this->x * 5 + (int)(Point::size / 2);
    int maxY = this->y * 5 + (int)(Point::size / 2);

    if (mouse_x >= minX && mouse_x <= maxX && mouse_y >= minY && mouse_y <= maxY)
        return true;
    else
        return false;
}
