#include <observer.h>
#include <iostream>

int main( int /* argc */, char * /* argv */[] )
{
    pg::connection_owner            owner;
    pg::subject< std::string, int > s;

    // All values from subject
    owner.connect( s, []( const std::string &str, int i )
    {
        std::cout << "Hello " << str << i << "!" << std::endl;
    } );

    // Only first value from subject
    owner.connect( s, []( const std::string &str )
    {
        std::cout << "Hello " << str << "!" << std::endl;
    } );

    // No value
    owner.connect( s, []
    {
        std::cout << "Hello!" << std::endl;
    } );

    s.notify( "PG", 1003 );

    return 0;
}
