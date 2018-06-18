#include "Network.h"
#include "iostream"

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

    // 遍历链表
    node = mVexs[start].firstEdge;
    while (node != NULL)
    {
        if (end == node->ivex)
            return node->capacity;
        node = node->nextEdge;
    }

    return (double)INF_N;
}

/**
 * 获取所有与point节点直连的节点
*/
std::set<Point *> Network::getDirectNodes(Point *point)
{
    ENode *node;
    std::set<Point *> result;

    // 遍历链表
    node = mVexs[getPosition(point->label)].firstEdge;
    while (node != NULL)
    {
        result.insert(mVexs[node->ivex].data);
        node = node->nextEdge;
    }

    return result;
}

/**
 * 判断end节点是否在start节点两跳范围内
*/
bool Network::isTwoSteps(Point *start, Point *end)
{
    bool result = false;
    // 获取节点索引
    int startPos = getPosition(start->label);
    int endPos = getPosition(end->label);

    // 如果有直接联系的边，则返回true
    if (getCapacity(startPos, endPos) > (double)INF_N)
    {
        result = true;
    }
    else
    {
        // 不能直达，则判断是否可以经过一个中间节点到达
        std::set<Point *> directStart = getDirectNodes(start);
        std::set<Point *> directEnd = getDirectNodes(end);

        // 如果两个集合存在重复元素则表示可间接连通
        std::set<Point *>::iterator it;
        for (it = directStart.begin(); it != directStart.end(); it++)
        {
            if (directEnd.find(*it) != directEnd.end())
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

/**
 * 调整历史流路径
*/
void Network::fixStreamPath(std::vector<std::stack<int>> streams)
{
    int i, j, k;
    int index;                     // 需要调整的流索引
    std::stack<int> path;          // 路径节点索引栈
    std::set<Point *> infs;        // 干扰源集合
    int len[MAX_NODE];             // 每条流的路径长度
    int route[MAX_NODE][MAX_NODE]; // 每条流的路径节点索引

    path = streams[streams.size() - 1]; // 取最新加入的流路径
    while (!path.empty())
    {
        // 除最后一个节点，其他节点均为干扰源
        if (path.size() > 1)
        {
            infs.insert(mVexs[path.top()].data);
            path.pop();
        }
    }

    // 将路径节点栈转化为二维数组
    for (i = 0, k = 0; i < Network::slen - 1; i++)
    {
        path = streams[i];
        while (!path.empty())
        {
            route[i][k++] = path.top();
            path.pop();
        }
        len[i] = k;
    }

    // 找到容量最小的流
    index = 0;
    for (i = 0; i < Network::slen - 1; i++)
    {
        double min = EData::calculate(mVexs[0].data, mVexs[1].data, true, infs);
        for (j = 1; j < len[i]; j++)
        {
            double cap = EData::calculate(mVexs[j].data, mVexs[j + 1].data, true, infs);
            if (cap < min)
            {
                min = cap;
                index = i;
            }
        }
    }

    // 重新规划路径
    Point *source = mVexs[route[index][0]].data;
    Point *dest = mVexs[route[index][len[index] - 1]].data;
    routing(source, dest, path);

    // 调整流顺序并更新整个网络链路容量
    for (i = Network::slen - 1; i > index; i--)
    {
        Network::streamPath[i - 1] = Network::streamPath[i];
    }
    Network::streamPath[Network::slen - 1] = path;

    if (Network::needUpdate)
        updateCapacity(Network::streamPath);
}

/*
 * 根据路由算法找出的路径更新各个链路的容量
 */
void Network::updateCapacity(std::vector<std::stack<int>> streams)
{
    int i, j, k;
    ENode *node;                // 需要更新容量的链路
    std::set<int> sendNodes;    // 所有流的路径中发射节点索引
    std::set<int> recNodes;     // 所有流的路径中接收节点索引
    std::set<int>::iterator it; // 集合迭代器

    // 节点信息预处理
    for (i = 0; i < Network::slen; i++)
    {
        // 路径节点索引数组
        j = 0;
        int route[MAX_NODE];
        std::stack<int> path = streams[i];

        // 将路径节点分类放入不同集合
        while (!path.empty())
        {
            int index = path.top();
            route[j++] = index;
            if (path.size() > 1)
            {
                // 如果在接收节点集合中存在，则将其删除
                it = recNodes.find(index);
                if (it != recNodes.end())
                    recNodes.erase(it);

                // 插入到发射节点集合
                sendNodes.insert(index);
            }
            else
            {
                // 插入到接收节点集合
                if (sendNodes.find(index) == sendNodes.end())
                    recNodes.insert(index);
            }
            path.pop();
        }

        // 将流中链路所用容量减掉
        for (k = 0; k < j - 1; k++)
        {
            node = mVexs[route[k]].firstEdge;
            while (node != NULL)
            {
                if (node->ivex == route[k + 1])
                {
                    node->capacity -= Network::capacity[i];
                    if (node->capacity <= (double)0.0)
                        node->capacity = (double)INF_N;
                }
                node = node->nextEdge;
            }
        }
    }

    // 更新受干扰源影响的链路容量，限干扰源两跳范围内的节点
    for (i = 0; i < mVexNum; i++)
    {
        // 仅更新起点不在发射节点集合中的链路
        if (sendNodes.find(i) == sendNodes.end())
        {
            node = mVexs[i].firstEdge;
            while (node != NULL)
            {
                if (i != node->ivex)
                {
                    // 仅更新终点不在信号流中的链路
                    if (sendNodes.find(node->ivex) == sendNodes.end() && recNodes.find(node->ivex) == recNodes.end())
                    {
                        Point *inf;   // 干扰源
                        Point *infed; // 被干扰节点
                        infed = mVexs[node->ivex].data;

                        // 在发射节点中寻找符合两跳范围的干扰源
                        std::set<Point *> infs;
                        for (it = sendNodes.begin(); it != sendNodes.end(); it++)
                        {
                            inf = mVexs[*it].data;
                            if (isTwoSteps(inf, infed))
                                infs.insert(inf);
                        }

                        // 根据干扰源重新计算信道容量
                        node->capacity = EData::calculate(mVexs[i].data, mVexs[*it].data, true, infs);
                    }
                }

                // 继续计算下一条链路
                node = node->nextEdge;
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
    // 将当前信号流和容量存入全局变量
    Network::slen++;
    Network::capacity.push_back(dist[ve]);
    Network::streamPath.push_back(path);

    // 计算干扰信号，更新各个链路容量
    if (Network::needUpdate)
        updateCapacity(Network::streamPath);
    return dist[ve];
}
