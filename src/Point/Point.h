#ifndef POINT_H
#define POINT_H

/**
 * 网络信号节点的结构
*/
class Point
{
#define MAX_NODE 100 // 最大节点数量
  public:
    int x;      // 节点横坐标
    int y;      // 节点纵坐标
    char label; // 节点名称

    static int vlen;              // 节点数量
    static int size;              // 节点大小
    static Point *vexs[MAX_NODE]; // 网络节点全局数组

  public:
    Point(int x, int y, char l) : x(x), y(y), label(l) {}
    // 获取和其他节点的直线距离
    double getDistance(Point *other);
    // 判断节点是否被选中
    bool selected(int mouse_x, int mouse_y);
};

#endif // !POINT_H
