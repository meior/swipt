#include <cstdlib>
#include <iostream>
using namespace std;

int main()
{
    int i;
    for (i = 0; i < 100; i++)
    {
        cout << rand() % 100 << endl;
    }

    return 1;
}
