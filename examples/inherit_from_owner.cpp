#include <observer.h>
#include <iostream>

struct bar : public pg::connection_owner
{
    bar( pg::subject< std::string >& foo )
    {
        connect( foo, this, &bar::print );
    }

    void print( const std::string &str )
    {
        std::cout << str << std::endl;
    }
};

int main( int /* argc */, char * /* argv */[] )
{
    pg::subject< std::string > foo;
    bar                        b( foo );

    foo.notify( "Hello World!" );

    return 0;
}
