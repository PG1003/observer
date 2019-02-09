#include <observer.h>
#include <iostream>
#include <functional>

struct functor
{
    void operator()()
    {
        std::cout << "Hello World!" << std::endl;
    }
};

int main( int /* argc */, char * /* argv */[] )
{
    pg::observer_owner owner;
    pg::subject        s;

    const std::function< void() > function = []{ std::cout << "Hello PG1003!" << std::endl; };

    // NOTE: 'functor' and 'function' are passed by value.
    //       This means they are copied into the observer
    owner.connect( s, functor() );
    owner.connect( s, function );

    s.notify();

    return 0;
}
