#include "EData.h"
#include "cmath"

/*
 * 生成网络链路
 */
EData::EData(Point *start, Point *end)
{
    std::set<Point *> temp;
    this->start = start;
    this->end = end;
    this->working = false;
    this->capacity = EData::calculate(start, end, false, temp);
}

/*
* 计算链路容量，区分初始状态与路由规划过程中的更新状态
*
* 参数说明：
*     start -- 链路起点
*       end -- 链路终点
*    update -- 是否是对容量进行更新
*      infs -- 干扰源索引集合
*/
double EData::calculate(Point *start, Point *end, bool update, std::set<Point *> infs)
{
    double dis;     // 节点距离
    double gain;    // 信道增益
    double noise_i; // 干扰噪声
    double per;     // 能量与信息处理分配百分比
    double snr;     // 信噪比
    double cap;     // 有效信息信道容量

    dis = start->getDistance(end);
    gain = 1 / (1 + pow(dis, 2.7));

    // 计算所有干扰源产生的噪声
    noise_i = 0;
    if (update)
    {
        std::set<Point *>::iterator it;
        for (it = infs.begin(); it != infs.end(); it++)
        {
            double dis_i = end->getDistance(*it);
            double gain_i = 1 / (1 + pow(dis_i, 2.7));
            noise_i += POWER * pow(gain_i, 2);
        }
    }

    // 计算信号分配比、信噪比、信道传输容量
    per = 1 - ENERGY / (RATE * (POWER * pow(gain, 2) + noise_i + NOISE1));
    snr = per * POWER * pow(gain, 2) / (per * NOISE1 + NOISE2 + per * noise_i);

    // 信噪比低于最低要求视为不可到达
    if (snr < MINSNR)
        cap = (double)INF_N;
    else
        cap = WIDTH * log10(1 + snr) / log10(2);
    return cap;
}
