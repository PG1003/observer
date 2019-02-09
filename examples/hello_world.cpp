#include <observer.h>
#include <iostream>

void hello_world()
{
    std::cout << "Hello World!" << std::endl;
}

int main( int /* argc */, char * /* argv */[] )
{
    pg::observer_owner owner;
    pg::subject        s;

    owner.connect( s, hello_world );

    s.notify();

    return 0;
}
