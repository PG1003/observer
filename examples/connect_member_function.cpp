#include <observer.h>
#include <iostream>

struct foo
{
    void bar()
    {
        std::cout << "Hello World!" << std::endl;
    }
};

int main( int /* argc */, char * /* argv */[] )
{
    pg::observer_owner owner;
    pg::subject<>      s;
    foo                f;

    owner.connect( s, &f, &foo::bar );

    s.notify();

    return 0;
}
