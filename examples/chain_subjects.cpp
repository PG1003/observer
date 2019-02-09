#include <observer.h>
#include <iostream>

int main( int /* argc */, char * /* argv */[] )
{
    pg::observer_owner owner;
    pg::subject        s1;
    pg::subject        s2;
    pg::subject        sy;
    pg::subject        s;

    // Connect sy both to s1 and s2
    owner.connect( s1, sy );
    owner.connect( s2, sy );

    // Redirect notifications from sy to s
    owner.connect( sy, s);

    owner.connect( s, []{ std::cout << "Hello World!" << std::endl; } );

    // Print "Hello World!" two times.
    s1.notify();
    s2.notify();

    return 0;
}
