#ifndef EDATA_H
#define EDATA_H

#include "../Point/Point.h"
#include "set"

/**
 * 链路的结构
*/
class EData
{
#define POWER 100.0        // 最大发射功率
#define ENERGY 0.01        // 最低能量需求
#define WIDTH 10e6         // 信道带宽
#define RATE 0.65          // 能量转化效率
#define NOISE1 1e-7        // 天线噪声
#define NOISE2 1e-5        // 信号处理噪声
#define MINSNR 0.1         // 最低信噪比
#define INF_N -10e6        // 最小容量
#define INF (~(0x1 << 31)) // 最大容量
  public:
    Point *start;    // 链路的起点
    Point *end;      // 链路的终点
    bool working;    // 链路是否有流在传输
    double capacity; // 链路的最大传输容量

    static int elen;                          // 链路数
    static EData *edges[MAX_NODE * MAX_NODE]; // 网络链路全局数组

  public:
    EData(Point *start, Point *end);
    // 计算链路容量，区分初始状态与路由规划过程中的更新状态
    static double calculate(Point *start, Point *end, bool update, std::set<Point *> infs);
};

#endif // !EDATA_H
