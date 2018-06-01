#include <iostream>
#define INF_N -10e6
using namespace std;

int main()
{
    int inf = (~(unsigned int)0 >> 1) + 1;
    cout << inf << endl;

    double cap1 = (double)INF_N;
    double cap2 = (double)INF_N;
    cout << (cap1 == cap2) << endl;

    return 0;
}