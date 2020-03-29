#include <observer.h>
#include <iostream>

void hello_world()
{
    std::cout << "Hello World!" << std::endl;
}

int main( int /* argc */, char * /* argv */[] )
{
    pg::subject<> s;

    auto connection = pg::connect( s, hello_world );

    s.notify();

    return 0;
}
