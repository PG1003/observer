# observer

A templated observer mechanism as a header-only library that is inspired by Qt's signals and slots.

## Features

Highlights of this library are:

* Defining the subject's notification values by variadic template parameters.
* Connect all kinds of callables to a subject like member functions, lambdas, functors and free functions.
* Connected callables can accept _less_ parameters than the subject provides.

## Requirements

* C++17 compliant compiller.

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
    pg::observer_owner owner;
    pg::subject        hello_subject;
    
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

This example shows some more advanced features of the library:
* Objects can inherit from ```pg::observer_owner```.
* Connecting member functions to a subject.
* Connecting lambdas to a subject.
* Connecting observer functions that accepts _less_ arguments than the subject's notify passes on.

``` c++
struct bar : public pg::observer_owner
{
    bar( pg::subject< std::string, int >& foo )
    {
        connect( foo, this, &bar::print );
        connect( foo, []( const std::string &str )
        {
            std::cout << "Hello " << str << "!" << std::endl;
        } );
    }

    void print( const std::string &str, int i )
    {
        std::cout << str << i << std::endl;
    }
};

int main( int /* argc */, char * /* argv */[] )
{
    pg::subject< std::string, int > foo;
    bar                             b( foo );

    foo.notify( "PG", 1003 );

    return 0;
}
```
The output is:
> PG1003    
> Hello PG!

