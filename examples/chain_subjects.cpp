#include <observer.h>
#include <iostream>

int main( int /* argc */, char * /* argv */[] )
{
    pg::connection_owner       owner;
    pg::subject<>              s1;
    pg::subject< const char* > s2;
    pg::subject<>              sy;
    pg::subject<>              s;

    // Connect sy both to s1 and s2
    owner.connect( s1, &sy, &pg::subject<>::notify );
    owner.connect( s2, &sy, &pg::subject<>::notify );

    // Redirect notifications from sy to s
    owner.connect( sy, &s, &pg::subject<>::notify );

    owner.connect( s, []{ std::cout << "Hello World!" << std::endl; } );

    // Print "Hello World!" two times.
    s1.notify();
    s2.notify( "PG1003" );

    return 0;
}
