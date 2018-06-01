#include <iostream>
#include <set>
using namespace std;

int main()
{
    set<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(1);

    cout << "size: " << s.size() << endl;
    cout << "maxsize: " << s.max_size() << endl;
    cout << "begin: " << *s.begin() << endl;
    cout << "end: " << *s.end() << endl;

    return 0;
}
