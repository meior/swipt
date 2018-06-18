#ifndef NETWORK_H
#define NETWORK_H

#include "../EData/EData.h"
#include "../Point/Point.h"
#include "set"
#include "stack"
#include "vector"

/**
 * 网络图结构的邻接表
*/
class Network
{
  private:
    /**
     * 邻接表中链表的节点
    */
    class ENode
    {
        int ivex;        // 该边所指向的节点的位置
        double capacity; // 该边的权
        ENode *nextEdge; // 指向下一条弧的指针
        friend class Network;
    };

    /**
     * 邻接表中链表的头节点
    */
    class VNode
    {
        Point *data;      // 节点信息
        ENode *firstEdge; // 指向第一条依附该节点的弧
        friend class Network;
    };

  private:
    int mVexNum;           // 图的节点的数目
    int mEdgNum;           // 图的边的数目
    VNode mVexs[MAX_NODE]; // 图的邻接表头数组

  public:
    static int slen;                                // 信号流数量
    static bool needUpdate;                         // 容量更新标记
    static Network *net;                            // 网络结构全局变量
    static std::vector<double> capacity;            // 路径最大传输容量
    static std::vector<std::stack<int>> streamPath; // 路径节点索引栈

  public:
    // 创建邻接表对应的图
    Network(Point *vexs[], int vlen, EData *edges[], int elen);
    // 路由算法
    double routing(Point *source, Point *dest, std::stack<int> &path);

  private:
    // 返回节点的位置
    int getPosition(char label);
    // 将node节点链接到list的最后
    void linkLast(ENode *list, ENode *node);
    // 获取边<start, end>的权值（容量）
    double getCapacity(int start, int end);
    // 获取所有与point节点直连的节点
    std::set<Point *> getDirectNodes(Point *point);
    // 判断end节点是否在start节点两跳范围内
    bool isTwoSteps(Point *start, Point *end);
    // 调整历史流路径
    void fixStreamPath(std::vector<std::stack<int>> streams);
    // 更新各个链路的容量
    void updateCapacity(std::vector<std::stack<int>> streams);
    // 获取具体信号流路径
    void getPath(int start, int end, int prev[], std::stack<int> &path);
};

#endif // !NETWORK_H
