# observer

An observer pattern that can ignore extra arguments like Qt's signals and slots.

## Features

* Connected callables can accept _less_ parameters than the subject's interface provides.
* Defining the observer's notification values by variadic template parameters.
* Connect all kinds of callables to a subject; member functions, lambdas, functors, ```std::function``` and free functions.
* Support for custum subjects via static polymorphism. 
* Lifetime management of the connection between the subject and observer. 
* [1 header file](https://github.com/PG1003/observer/blob/master/src/observer.h) that includes only 3 headers from the standard template library.

## Requirements

* C++14 compliant compiller. 

## Examples

In the [examples folder](https://github.com/PG1003/observer/blob/master/examples) you will find example programs that show the features and usage of this library.
You can also take a peek in [tests.cpp](https://github.com/PG1003/observer/blob/master/test/tests.cpp).

The next two examples shown here are a short introduction of this library.

### Example 1

This example shows the basic usage of the library.

``` c++
void hello_function()
{
    std::cout << "Hello World!" << std::endl;
}

int main( int /* argc */, char * /* argv */[] )
{
    pg::connection_owner owner;
    pg::subject<>        hello_subject;
    
    owner.connect( hello_subject, hello_function );
    
    hello_subject.notify();
    
    return 0;
}
```
The output is:
>Hello World!

So, what happened here?

There are two objects declared in the ```main``` function:
* ```owner``` manages lifetime of the connection between subjects and observers.
* ```hello_subject``` which will be used to emit notifications to observers that are connected.

Next, the owner connects ```hello_function``` observer function to the ```hello_subject```.

When ```hello_subject``` does a ```notify``` then all observer functions that are connected are called.

### Example 2

This example shows three more features of the library:
* Connecting a lambda to a subject that accepts _less_ arguments than the subject's notify passes on.
* Connecting a member function to a subject.
* Disconnecting a function.

``` c++
struct b : public pg::connection_owner
{
    void print( const std::string &str, int i )
    {
        std::cout << str << i << std::endl;
    }
};

int main( int /* argc */, char * /* argv */[] )
{
    pg::connection_owner            owner;
    pg::subject< std::string, int > foo;
    b                               bar;

    owner.connect( foo, []( const std::string &str )
    {
        std::cout << "Hello " << str << '!' << std::endl;
    } );

    auto connection = owner.connect( foo, &bar, &b::print );

    foo.notify( "PG", 1003 );

    owner.disconnect( connection );

    foo.notify( "PG", 1003 );

    return 0;
}
```
The output is:
> Hello PG!  
> PG1003  
> Hello PG!

## C++17 CTAD

Although C++14 is required, C++17 introduced [CTAD](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction) which simplifies the use of ```pg::subject_blocker``` and makes defining a parameterless ```pg::subject``` prettier.

``` c++
pg::subject<> foo;  // C++14
pg::subject   foo;  // C++17

pg::subject_blocker< pg::subject<> > blocker( foo );   // C++14
pg::subject_blocker                  blocker( foo );   // C++17
```
