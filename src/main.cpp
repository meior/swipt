#include "./Network/Network.h"
#include "GL/glut.h"
#include "cstdlib"
#include "ctime"
#include "iostream"
#include "string"
#define MAX_CHAR 128

/**
 * 屏幕高度、屏幕宽度、节点大小、直线宽度
*/
GLsizei window_h = 600;
GLsizei window_w = 600;
GLint Point::size = 10;
GLint lineWidth = 3;

/**
 * 节点数、链路数、信号流数
*/
int Point::vlen;
int EData::elen;
int Network::slen;

/**
 * 鼠标选中的起点与终点标记
*/
int routeSource = -1;
int routeDest = -1;

/**
 * 路径节点索引栈
*/
std::vector<std::stack<int>> Network::streamPath;

/**
 * 路径最大传输容量
*/
std::vector<double> Network::capacity;

/**
 * 容量更新标记
*/
bool Network::needUpdate = true;

/**
 * 网络节点全局数组
*/
Point *Point::vexs[MAX_NODE];

/**
 * 网络链路全局数组
*/
EData *EData::edges[MAX_NODE * MAX_NODE];

/**
 * 网络结构全局变量
*/
Network *Network::net;

/**
 * 初始化所有链路
*/
void initEdges()
{
    int i, j, k;
    for (i = 0, k = 0; i < Point::vlen; i++)
    {
        for (j = 0; j < Point::vlen; j++)
        {
            // 排除到自身的链路
            if (i != j)
                EData::edges[k++] = new EData(Point::vexs[i], Point::vexs[j]);
        }
    }
}

/**
 * 初始化网络结构
*/
void initNetwork(int argc, char **argv)
{
    // 构造网络节点数据
    Point::vlen = 0;
    Point::vexs[Point::vlen++] = new Point(110, 18, 'A');
    Point::vexs[Point::vlen++] = new Point(14, 17, 'B');
    Point::vexs[Point::vlen++] = new Point(110, 70, 'C');
    Point::vexs[Point::vlen++] = new Point(27, 40, 'D');
    Point::vexs[Point::vlen++] = new Point(42, 102, 'E');
    Point::vexs[Point::vlen++] = new Point(77, 37, 'F');
    Point::vexs[Point::vlen++] = new Point(16, 70, 'G');
    Point::vexs[Point::vlen++] = new Point(67, 70, 'H');
    Point::vexs[Point::vlen++] = new Point(42, 46, 'I');

    // 获取是否更新容量标记
    if (argc >= 2 && *argv[1] == '0')
        Network::needUpdate = false;

    // 初始化理想态下所有链路
    EData::elen = Point::vlen * (Point::vlen - 1);
    initEdges();

    // 初始化流数量
    Network::slen = 0;
    // 构造网络图结构
    Network::net = new Network(Point::vexs, Point::vlen, EData::edges, EData::elen);
}

/**
 * 绘制节点
*/
void drawPoint(Point *point)
{
    glPointSize(Point::size);
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
    std::stack<int> path;

    // 初始化链路标记
    //initEdges();

    // 仅当路径不为空且有效才绘制
    if (!Network::capacity.empty() && Network::capacity[Network::slen - 1] != INF_N)
    {
        // 将节点栈转化为数组
        path = Network::streamPath[Network::slen - 1];
        while (!path.empty())
        {
            temp[len++] = path.top();
            path.pop();
        }

        // 设定该路径统一的RGB颜色，各值随机产生
        srand((int)time(0));
        float red = (float)(rand() % 100) / 100;
        float green = (float)(rand() % 100) / 100;
        float blue = (float)(rand() % 100) / 100;

        // 依次取两个节点进行绘制
        for (i = 0; i < len - 1; i++)
        {
            Point *start = Point::vexs[temp[i]];
            Point *end = Point::vexs[temp[i + 1]];

            // 找到这条边，标记为正在传输
            for (j = 0; j < EData::elen; j++)
            {
                if (EData::edges[j]->start == start && EData::edges[j]->end == end)
                {
                    // 每条路径使用不同颜色以区分，存入路径对应的位置
                    EData::edges[j]->color[Network::slen - 1][0] = red;
                    EData::edges[j]->color[Network::slen - 1][1] = green;
                    EData::edges[j]->color[Network::slen - 1][2] = blue;
                    // 标记正在使用该链路的路径
                    EData::edges[j]->used.insert(Network::slen - 1);
                }
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
    std::string info; // 用于显示的文本信息

    // 绘制模式标记文字
    if (Network::needUpdate)
        info = "INTERFERENCE MODE";
    else
        info = "NO-INTERFERENCE MODE";
    glColor3f(0.0f, 1.0f, 0.0f);
    glRasterPos2i(window_w / 2 - 70, window_h - 30);
    drawString(info.data());

    // 绘制最大传输容量，如果路径无效则提示不可达
    if (Network::capacity.empty())
    {
        info = "max capacity: " + std::to_string((double)0.0) + "Mbps";
        glColor3f(0.0f, 0.0f, 0.0f);
    }
    else
    {
        if (Network::capacity[Network::slen - 1] == INF_N)
        {
            info = "unreachable !";
            glColor3f(1.0f, 0.0f, 0.0f);
        }
        else
        {
            info = "max capacity: " + std::to_string(Network::capacity[Network::slen - 1] / 10e6) + "Mbps";
            glColor3f(0.0f, 0.0f, 0.0f);
        }
    }
    glRasterPos2i(window_w / 2 - 70, window_h - 55);
    drawString(info.data());

    // 绘制标题区与操作区分割线
    Point *left = new Point(0, (window_h - 70) / 5, 'X');
    Point *right = new Point(window_w / 5, (window_h - 70) / 5, 'Y');
    glColor3f(0.0f, 0.0f, 0.0f);
    drawLine(left, right);

    // 绘制网络节点
    for (i = 0; i < Point::vlen; i++)
    {
        if (i == routeSource || i == routeDest)
            glColor3f(0.0f, 1.0f, 0.0f);
        else
            glColor3f(0.0f, 0.0f, 0.0f);
        drawPoint(Point::vexs[i]);

        // 绘制节点文本信息
        info = Point::vexs[i]->label;
        info.append("(" + std::to_string(Point::vexs[i]->x) + " , " + std::to_string(Point::vexs[i]->y) + ")");
        glColor3f(0.0f, 0.0f, 1.0f);
        glRasterPos2i(Point::vexs[i]->x * 5 - 30, Point::vexs[i]->y * 5 - 20);
        drawString(info.data());
    }

    // 绘制所有网络路径
    for (i = 0; i < Network::slen; i++)
    {
        for (j = 0; j < EData::elen; j++)
        {
            if (EData::edges[j]->used.find(i) != EData::edges[j]->used.end())
            {
                glColor3fv(EData::edges[j]->color[i]);
                drawLine(EData::edges[j]->start, EData::edges[j]->end);
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
            for (i = 0; i < Point::vlen; i++)
            {
                if (Point::vexs[i]->selected(mouse_x, window_h - mouse_y))
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
                std::stack<int> path;
                Network::net->routing(Point::vexs[routeSource], Point::vexs[routeDest], path);

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
            for (i = 0; i < Point::vlen; i++)
            {
                if (Point::vexs[i]->selected(mouse_x, window_h - mouse_y))
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
