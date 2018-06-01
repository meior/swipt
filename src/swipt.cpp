#include <GL/glut.h>
#include <cmath>
#include <iostream>
#include <set>
#include <stack>
#include <string>
#include <vector>
#define INF (~(0x1 << 31))
#define INF_N -10e6
#define MAX_NODE 100
#define MAX_CHAR 128
using namespace std;

/**
 * 屏幕高度、屏幕宽度、节点大小、直线宽度
*/
GLsizei window_h = 600;
GLsizei window_w = 600;
GLint pointSize = 10;
GLint lineWidth = 3;

/**
 * 节点数、链路数、信号流数
*/
int vlen = 0;
int elen = 0;
int slen = 0;

/**
 * 鼠标选中的起点与终点标记
*/
int routeSource = -1;
int routeDest = -1;

/**
 * 路径节点索引栈
*/
vector<stack<int>> streamPath;

/**
 * 当前网络路径最大传输容量
*/
double capacity = 0.0;

/**
 * 容量更新标记
*/
bool needUpdate = true;

/**
 * 网络信号节点的结构
*/
class Point
{
  public:
    GLint x;    // 节点横坐标
    GLint y;    // 节点纵坐标
    char label; // 节点名称

  public:
    Point(GLint x, GLint y, char l) : x(x), y(y), label(l) {}
    // 获取和其他节点的直线距离
    double getDistance(Point *other);
    // 判断节点是否被选中
    bool selected(GLint mouse_x, GLint mouse_y);
};

/**
 * 网络节点全局变量
*/
Point *vexs[] = {
    new Point(51, 84, 'A'),
    new Point(32, 25, 'B'),
    new Point(63, 49, 'C'),
    new Point(93, 95, 'D'),
    new Point(21, 97, 'E'),
    new Point(99, 80, 'F'),
    new Point(85, 16, 'G'),
};

/**
 * 链路的结构
*/
class EData
{
#define POWER 100.0 // 最大发射功率
#define ENERGY 0.01 // 最低能量需求
#define WIDTH 10e6  // 信道带宽
#define RATE 0.65   // 能量转化效率
#define NOISE1 1e-7 // 天线噪声
#define NOISE2 1e-5 // 信号处理噪声
#define MINSNR 0.1  // 最低信噪比
  public:
    Point *start;    // 链路的起点
    Point *end;      // 链路的终点
    bool working;    // 链路是否有流在传输
    double capacity; // 链路的最大传输容量

  public:
    EData(Point *start, Point *end);
    // 计算链路容量，区分初始状态与路由规划过程中的更新状态
    static double calculate(Point *start, Point *end, bool update, set<Point *> infs);
};

/**
 * 网络链路全局变量
*/
EData *edges[MAX_NODE * MAX_NODE];

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
    // 创建邻接表对应的图
    Network(Point *vexs[], int vlen, EData *edges[], int elen);
    // 路由算法
    double routing(Point *source, Point *dest, stack<int> &path);

  private:
    // 返回节点的位置
    int getPosition(char label);
    // 将node节点链接到list的最后
    void linkLast(ENode *list, ENode *node);
    // 获取边<start, end>的权值（容量）
    double getCapacity(int start, int end);
    // 更新各个链路的容量
    void updateCapacity(vector<stack<int>> streams);
    // 获取具体信号流路径
    void getPath(int start, int end, int prev[], stack<int> &path);
};

/**
 * 网络结构全局变量
*/
Network *net;

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
bool Point::selected(GLint mouse_x, GLint mouse_y)
{
    GLint minX = this->x * 5 - (GLint)(pointSize / 2);
    GLint minY = this->y * 5 - (GLint)(pointSize / 2);
    GLint maxX = this->x * 5 + (GLint)(pointSize / 2);
    GLint maxY = this->y * 5 + (GLint)(pointSize / 2);

    if (mouse_x >= minX && mouse_x <= maxX && mouse_y >= minY && mouse_y <= maxY)
        return true;
    else
        return false;
}

/*
 * 生成网络链路
 */
EData::EData(Point *start, Point *end)
{
    set<Point *> temp;
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
double EData::calculate(Point *start, Point *end, bool update, set<Point *> infs)
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
        set<Point *>::iterator it;
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
void Network::updateCapacity(vector<stack<int>> streams)
{
    int i, j, k, l, m;
    int len[MAX_NODE];             // 所有流的路径长度
    int route[MAX_NODE][MAX_NODE]; // 所有流的路径经过的节点索引
    set<int> nodes;                // 所有流的路径中涉及到的节点索引
    ENode *node;                   // 需要更新容量的链路

    // 将路径节点的索引按信息流动顺序存入数组
    for (i = 0, l = 0; i < slen; i++, l = 0)
    {
        stack<int> path = streams[i];

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
    set<int>::iterator it;
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

                // 找到*it两跳范围内存在干扰的节点
                set<Point *> infs;
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

                node->capacity = EData::calculate(mVexs[i].data, mVexs[*it].data, true, infs);
            }
        }
    }
}

/*
 * 根据路由算法计算出的前驱数组倒推出具体路径
 */
void Network::getPath(int start, int end, int prev[], stack<int> &path)
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
double Network::routing(Point *source, Point *dest, stack<int> &path)
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

/**
 * 初始化所有链路
*/
void initEdges()
{
    int i, j, k;
    for (i = 0, k = 0; i < vlen; i++)
    {
        for (j = 0; j < vlen; j++)
        {
            // 排除到自身的链路
            if (i != j)
                edges[k++] = new EData(vexs[i], vexs[j]);
        }
    }
}

/**
 * 初始化网络结构
*/
void initNetwork(int argc, char **argv)
{
    // 获取是否更新容量标记
    if (argc >= 2 && *argv[1] == '0')
        needUpdate = false;

    // 初始化理想态下所有链路
    vlen = sizeof(vexs) / sizeof(vexs[0]);
    elen = vlen * (vlen - 1);
    initEdges();

    // 构造网络图结构
    net = new Network(vexs, vlen, edges, elen);
}

/**
 * 绘制节点
*/
void drawPoint(Point *point)
{
    glPointSize(pointSize);
    glBegin(GL_POINTS);
    glVertex2i(point->x * 5, point->y * 5);
    glEnd();
}

/**
 * 在节点之间绘制直线
*/
void drawLine(Point *start, Point *end)
{
    glLineWidth(lineWidth);
    glBegin(GL_LINES);

    glVertex2i(start->x * 5, start->y * 5);
    glVertex2i(end->x * 5, end->y * 5);
    glEnd();
}

/**
 * 绘制文字
*/
void drawString(const char *string)
{
    static int isFirstCall = 1;
    static GLuint lists;

    if (isFirstCall)
    {
        isFirstCall = 0;
        // 申请连续的显示列表编号
        lists = glGenLists(MAX_CHAR);
        // 把每个字符的绘制命令都装到对应的显示列表中
        wglUseFontBitmaps(wglGetCurrentDC(), 0, MAX_CHAR, lists);
    }

    // 调用每个字符对应的显示列表，绘制每个字符
    while (*string != '\0')
    {
        glCallList(lists + *string);
        string++;
    }
}

/**
 * 绘制完整流路径
*/
void paintPath(int value)
{
    // 从路径栈读取链路
    int i, j;
    int len = 0;
    int temp[MAX_NODE];
    stack<int> path;

    // 初始化链路标记
    initEdges();

    // 仅当路径有效才绘制
    if (capacity != INF_N)
    {
        // 将节点栈转化为数组
        path = streamPath[slen - 1];
        while (!path.empty())
        {
            temp[len++] = path.top();
            path.pop();
        }

        // 依次取两个节点进行绘制
        for (i = 0; i < len - 1; i++)
        {
            Point *start = vexs[temp[i]];
            Point *end = vexs[temp[i + 1]];

            // 找到这条边，标记为正在传输
            for (j = 0; j < elen; j++)
            {
                if (edges[j]->start == start && edges[j]->end == end)
                    edges[j]->working = true;
            }
        }
    }

    // 还原标记并触发重绘
    routeSource = -1;
    routeDest = -1;
    glutPostRedisplay();
}

/**
 * 窗口绘制响应函数
*/
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    int i, j;
    string info; // 用于显示的文本信息
    for (i = 0; i < vlen; i++)
    {
        // 绘制模式标记文字
        if (needUpdate)
            info = "INTERFERENCE MODE";
        else
            info = "NO-INTERFERENCE MODE";
        glColor3f(0.0f, 1.0f, 0.0f);
        glRasterPos2i(window_w / 2 - 70, window_h - 30);
        drawString(info.data());

        // 绘制最大传输容量，如果路径无效则提示不可达
        if (capacity == INF_N)
        {
            info = "unreachable !";
            glColor3f(1.0f, 0.0f, 0.0f);
        }
        else
        {
            info = "max capacity: " + to_string(capacity / 10e6) + "Mbps";
            glColor3f(0.0f, 0.0f, 0.0f);
        }
        glRasterPos2i(window_w / 2 - 70, window_h - 60);
        drawString(info.data());

        // 绘制网络节点
        if (i == routeSource || i == routeDest)
            glColor3f(0.0f, 1.0f, 0.0f);
        else
            glColor3f(0.0f, 0.0f, 0.0f);
        drawPoint(vexs[i]);

        // 绘制节点文本信息
        info = vexs[i]->label;
        info.append("(" + to_string(vexs[i]->x) + " , " + to_string(vexs[i]->y) + ")");
        glColor3f(0.0f, 0.0f, 1.0f);
        glRasterPos2i(vexs[i]->x * 5 - 30, vexs[i]->y * 5 - 20);
        drawString(info.data());

        // 绘制网络链路
        for (j = 0; j < elen; j++)
        {
            if (edges[j]->working)
            {
                glColor4f(1.0f, 0.0f, 0.0f, 0.0f);
                drawLine(edges[j]->start, edges[j]->end);
            }
        }
    }

    // 刷新窗口
    glFlush();
    glutSwapBuffers();
}

/**
 * 窗口形状变化响应函数
*/
void reshape(int width, int height)
{
    // 重置视口大小，元素不自动缩放
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)width, 0.0, (GLdouble)height);

    // 重置窗口大小
    window_w = width;
    window_h = height;
}

/**
 * 键盘事件响应函数
*/
void keyboard(GLubyte key, GLint mouse_x, GLint mouse_y)
{
    switch (key)
    {
    // 按下esc键则关闭窗口
    case '\x1B':
        exit(EXIT_SUCCESS);
        break;
    }
}

/**
 * 鼠标点击事件响应函数
*/
void mouse(GLint button, GLint action, GLint mouse_x, GLint mouse_y)
{
    int i;
    // 鼠标左键
    if (button == GLUT_LEFT_BUTTON)
    {
        // 按下
        if (action == GLUT_DOWN)
        {
            for (i = 0; i < vlen; i++)
            {
                if (vexs[i]->selected(mouse_x, window_h - mouse_y))
                {
                    if (routeSource < 0)
                        routeSource = i;
                    else
                        routeDest = i;
                    break;
                }
            }

            // 选中两个节点则开始绘制路径
            if (routeSource >= 0 && routeDest >= 0)
            {
                stack<int> path;
                capacity = net->routing(vexs[routeSource], vexs[routeDest], path);

                // 延迟300ms绘制完整流路径
                glutTimerFunc(300, paintPath, 1);
            }
        }
    }

    // 鼠标右键
    if (button == GLUT_RIGHT_BUTTON)
    {
        // 按下
        if (action == GLUT_DOWN)
        {
            for (i = 0; i < vlen; i++)
            {
                if (vexs[i]->selected(mouse_x, window_h - mouse_y))
                {
                    // 取消选中起点
                    if (routeSource >= 0 && i == routeSource)
                    {
                        routeSource = routeDest;
                        routeDest = -1;
                    }

                    // 取消选中终点
                    if (routeDest >= 0 && i == routeDest)
                        routeDest = -1;

                    break;
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    // 设置窗口的缓存和颜色模型
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    // 设置窗口左上角的位置
    glutInitWindowPosition(400, 200);
    // 设置窗口的宽高
    glutInitWindowSize(window_w, window_h);
    glutCreateWindow("SWIPT Routing Algorithm");

    // 设置显示窗口的背景为白色
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    // 设置投影类型为正投影
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)window_w, 0.0, (GLdouble)window_h);

    // 初始化网络图结构
    initNetwork(argc, argv);

    // 显示窗口
    glutDisplayFunc(&display);
    // 监听窗口变化
    glutReshapeFunc(&reshape);
    // 监听键盘事件
    glutKeyboardFunc(&keyboard);
    // 监听鼠标事件
    glutMouseFunc(&mouse);
    // 进入事件循环
    glutMainLoop();

    return EXIT_SUCCESS;
}
