#include <observer.h>
#include <iostream>

int main( int /* argc */, char * /* argv */[] )
{
    pg::observer_owner owner;
    pg::subject<>      s;

    const auto handle = owner.connect( s, []{ std::cout << "Hello World!" << std::endl; } );

    s.notify();  // Prints "Hello World!"

    owner.disconnect( handle );

    s.notify();  // Prints nothing

    return 0;
}
