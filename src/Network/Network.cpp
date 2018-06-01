#include "Network.h"

/*
 * 创建邻接表对应的图
 */
Network::Network(Point *vexs[], int vlen, EData *edges[], int elen)
{
    Point *c1, *c2;
    int i, p1, p2;
    double capacity;
    ENode *node1, *node2;

    // 初始化节点数和边数
    mVexNum = vlen;
    mEdgNum = elen;
    // 初始化邻接表的节点
    for (i = 0; i < mVexNum; i++)
    {
        mVexs[i].data = vexs[i];
        mVexs[i].firstEdge = NULL;
    }

    // 初始化邻接表的边
    for (i = 0; i < mEdgNum; i++)
    {
        if (edges[i]->capacity > (double)INF_N)
        {
            // 读取边的起始节点和结束节点
            c1 = edges[i]->start;
            c2 = edges[i]->end;
            capacity = edges[i]->capacity;

            p1 = getPosition(c1->label);
            p2 = getPosition(c2->label);

            node1 = new ENode();
            node1->ivex = p2;
            node1->capacity = capacity;
            // 将node1链接到p1所在链表的末尾
            if (mVexs[p1].firstEdge == NULL)
                mVexs[p1].firstEdge = node1;
            else
                linkLast(mVexs[p1].firstEdge, node1);

            node2 = new ENode();
            node2->ivex = p1;
            node2->capacity = capacity;
            // 将node2链接到p2所在链表的末尾
            if (mVexs[p2].firstEdge == NULL)
                mVexs[p2].firstEdge = node2;
            else
                linkLast(mVexs[p2].firstEdge, node2);
        }
    }
}

/*
 * 根据节点标签返回节点的位置
 */
int Network::getPosition(char label)
{
    int i;
    for (i = 0; i < mVexNum; i++)
    {
        if (mVexs[i].data->label == label)
            return i;
    }
    return -1;
}

/*
 * 将node节点链接到list的最后
 */
void Network::linkLast(ENode *list, ENode *node)
{
    ENode *p = list;

    while (p->nextEdge)
    {
        p = p->nextEdge;
    }
    p->nextEdge = node;
}

/*
 * 获取边<start, end>的权值，若start和end不连通则返回无穷小
 */
double Network::getCapacity(int start, int end)
{
    ENode *node;

    if (start == end)
        return (double)INF;

    node = mVexs[start].firstEdge;
    while (node != NULL)
    {
        if (end == node->ivex)
            return node->capacity;
        node = node->nextEdge;
    }

    return (double)INF_N;
}

/*
 * 根据路由算法找出的路径更新各个链路的容量
 */
void Network::updateCapacity(std::vector<std::stack<int>> streams)
{
    int i, j, k, l, m;
    int len[MAX_NODE];             // 所有流的路径长度
    int route[MAX_NODE][MAX_NODE]; // 所有流的路径经过的节点索引
    std::set<int> nodes;           // 所有流的路径中涉及到的节点索引
    ENode *node;                   // 需要更新容量的链路

    // 将路径节点的索引按信息流动顺序存入数组
    for (i = 0, l = 0; i < slen; i++, l = 0)
    {
        std::stack<int> path = streams[i];

        len[i] = path.size();
        while (!path.empty())
        {
            int index = path.top();
            if (l > 0)
                nodes.insert(index);
            route[i][l++] = index;
            path.pop();
        }
    }

    // 更新以节点为终点的链路容量，干扰源限定为两跳范围内
    std::set<int>::iterator it;
    for (it = nodes.begin(); it != nodes.end(); it++)
    {
        for (i = 0; i < mVexNum; i++)
        {
            if (*it != i)
            {
                // 找到以i为起点、*it为终点的链路
                node = mVexs[i].firstEdge;
                while (node != NULL)
                {
                    if (*it == node->ivex)
                        break;
                    node = node->nextEdge;
                }

                // 当该链路是连通时才更新容量
                if (node != NULL)
                {
                    // 找到*it两跳范围内存在干扰的节点
                    std::set<Point *> infs;
                    for (j = 0; j < slen; j++)
                    {
                        // 确定*it在一条流中的位置
                        for (k = 0; k < len[j]; k++)
                        {
                            if (*it == route[j][k])
                                break;
                        }

                        // 从该位置向起点方向找两跳范围内的节点
                        for (m = 0; m < k && m < 2; m++)
                        {
                            infs.insert(mVexs[route[j][k - m - 1]].data);
                        }
                    }

                    // 根据干扰源重新计算信道容量
                    node->capacity = EData::calculate(mVexs[i].data, mVexs[*it].data, true, infs);
                }
            }
        }
    }
}

/*
 * 根据路由算法计算出的前驱数组倒推出具体路径
 */
void Network::getPath(int start, int end, int prev[], std::stack<int> &path)
{
    path.push(end);
    end = prev[end];

    while (end != start)
    {
        path.push(end);
        end = prev[end];
    }
    path.push(start);
}

/*
 * 路由算法，获取从起始节点到其他各个节点的最大传输容量路径
 *
 * 参数说明：
 *     source -- 起始节点，即信号发射节点
 *     dest -- 目标节点，即信号最终接收的目的节点
 *     path -- 路径结果，即路径经过的节点索引
 */
double Network::routing(Point *source, Point *dest, std::stack<int> &path)
{
    int i, j, k;
    double max, tmp;
    int flag[MAX_NODE];    // flag[i]=1表示起始节点vs到节点i的最大传输容量路径已成功获取
    int prev[MAX_NODE];    // 前驱节点索引数组，即起始节点vs到节点i的路径所经历的全部节点中，位于节点i之前的那个节点
    double dist[MAX_NODE]; // 长度数组，即起始节点vs到节点i的最大传输容量，由最小链路容量决定

    // 获取节点索引
    int vs = getPosition(source->label);
    int ve = getPosition(dest->label);

    // 初始化
    for (i = 0; i < mVexNum; i++)
    {
        flag[i] = 0;                  // 节点i的最大传输容量路径还没获取
        prev[i] = vs;                 // 节点i的前驱节点为起点
        dist[i] = getCapacity(vs, i); // 节点i的最大传输容量为节点vs到节点i的权
    }

    // 对节点vs自身进行初始化
    flag[vs] = 1;
    dist[vs] = INF;

    // 遍历mVexNum-1次，每次找出一个节点的最大传输容量路径
    for (i = 1; i < mVexNum; i++)
    {
        // 寻找当前最大传输容量路径
        // 即在未获取最大传输容量路径的节点中，找到离vs最远的节点k
        max = INF_N;
        for (j = 0; j < mVexNum; j++)
        {
            if (flag[j] == 0 && dist[j] > max)
            {
                max = dist[j];
                k = j;
            }
        }
        // 标记节点k为已获取到最大传输容量路径
        flag[k] = 1;

        // 修正当前最大传输容量路径和前驱节点
        // 即比较经过节点k后传输容量是否变得更大，如果能取得更大的传输容量则将节点k更新到路径中
        for (j = 0; j < mVexNum; j++)
        {
            tmp = getCapacity(k, j);
            // 路径的最大传输容量由最小链路容量决定
            tmp = (tmp == INF_N ? INF_N : (max < tmp ? max : tmp));
            if (flag[j] == 0 && (tmp > dist[j]))
            {
                dist[j] = tmp;
                prev[j] = k;
            }
        }
    }

    // 从前驱节点数组获取路径节点栈
    getPath(vs, ve, prev, path);
    // 将当前信号流存入全局变量
    slen++;
    streamPath.push_back(path);

    // 计算干扰信号，更新各个链路容量
    if (needUpdate)
        updateCapacity(streamPath);
    return dist[ve];
}
