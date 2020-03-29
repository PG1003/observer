# observer

An observer pattern that can ignore extra arguments like Qt's signals and slots.

## Features

* Connected callables can accept _less_ parameters than the subject's interface provides.
* Defining the observer's notification values by variadic template parameters.
* Connect all kinds of callables to a subject; member functions, lambdas, functors, ```std::function``` and free functions.
* Lifetime management of the connection between the subject and observer. 
* [1 header file](https://github.com/PG1003/observer/blob/master/src/observer.h) that includes only 2 headers from the standard template library.

## Requirements

* C++14 compliant compiller. 

## Goals

* Easy to use.
* Robust lifetime management.
* Low overhead.
* Customazation.

## Non-goals

* Processing return values returned by the connected functions and observers. The return value is ignored for most cases where an observer pattern or signal/slot solution is used.
  It also limit the usage of this library and adds complexity; all observers or connected functions must return the same type and the results must becombined when receiving values from multiple observers.
* Build-in mutexes/atomics/thread safety. Adding these will add complexity and impact the performance for cases where these are not needed.
  When needed you can create custom subjects that integrates with your application.
  
  When you really need these features out-of-the-box then you should try [boost signals](https://www.boost.org/doc/libs/1_72_0/doc/html/signals2.html).

## Documentation

### Observer

An observer is an object that implements an observer interface.
The ```connect``` functions of this observer library implement observer interfaces when connecting member functions or callables to a subject.
This saves you from implementing observer interfaces manually.

You use this interface when implementing custom subjects and observers.

#### Custom observers

It is possible to implement your own observer types by inheriting from the observer interface.
However then you should also handle the cases when a subject disconnect its observers, for example when a subject goes out of scope.

The variadic template arguments of the observer interface defines the value types that an observer should receive at notification.

```c++
class my_observer final : public pg::observer< int >
{
public:
    virtual void disconnect() override;         // Called by the subject when it disconnect its observers.
    virtual void notify( int arg ) override;    // Called when the subject notifies its observers.
};
```

### Subject

A subject is an object that notifies its observers.

This observer library contains two subject types; ```pg::subject``` and ```pg::blockable_subject```.
The latter of these two has a mechanism to temporary block notifications.

For both subjects types you can define with variadic template parameters the value types to pass when notifying the observers.
These template parameters also defines the observer interface which you can connect to the subject.

```c++
pg::subject< int, const char * >  // A subject that notifies an integer and a string, accepts pg::observer< int, const char * > observers interfaces 
pg::subject<>                     // A subject without prameters, accepts pg::observer<> observer interfaces
```

#### Custom subjects

To create custom subjects you need to define the following two member functions to connect observers and facilitate lifetime management:
* ```[discarded] connect( pg::observer< T... > * ) [const]```
* ```[discarded] disconnect( [const] pg::observer< T... > * ) [const]```

Also you must call for each observer the ```pg::observer< T... >::disconnect``` function before removing it from object, for example in the destructor.

```c++
class my_subject
{
    std::vector< pg::observer< my_data > * > m_observers;
    
public:    
    ~subject_base() noexcept
    {
        for( o : m_observers )
        {
            o->disconnect();
        }
    }

    void connect( pg::observer< my_data > * const o ) noexcept
    {
        m_observers.push_back( o );
    }

    void disconnect( pg::const observer< my_data > * const o ) noexcept
    {
        auto it_find = std::find( m_observers.cbegin(), m_observers.cend(), o );
        if( it_find != m_observers.cend() )
        {
            m_observers.erase( ( it_find ) );
        }
    }
};
```

### Connection lifetime management

Lifetime management of the connection between subjects and observers is important to release resources that are no longer needed or avoid accessing resources that are no longer available.
When a subject is removed, all connections to that subject must be removed too.
The same applies when removing an observer; the observer must be disconnected from its subject. 
 
There are two methods to manage the lifetime of connections between subjects and observers:
* Scoped connection
* Connection owner

#### Scoped connections

A scoped connection is a lightweight object of the ```pg::scoped_connection``` type which owns a connection.
The lifetime of the connection ends when the lifetime of the subject or when the scoped connection object ends.

Scoped connections are returned by the ```pg::connect``` functions when connecting a member function or a callable to a subject. 
These connections can be used at places where a limited number of connections are maintained.

```c++
pg::subject< int > s;

{
    // Create a scoped connection 
    pg::scoped_connection connection = pg::connect( s, []( int i ){ std::cout << i << std::endl; } );
    
    s.notify( 42 ); // Prints '42' in the output
}

s.notify( 1337 );   // Prints nothing, connection went out of scope
```

#### Connection owner

A connection owner is usefull for places where a lot of connections need to be managed.
You can add a connection owner via composition by adding a ```pg::connection_owner``` member or by deriving from it.
The lifetime of the connections owned by the connection owner end when the lifetime of the subject or when the connection owner object itself ends.

A connection owner object owns only connections that are created with one of its ```pg::connection_owner::connect``` functions.
Connection owners doe not share connections.

```c++
pg::subject< int > s;

{
    pg::connection_owner owner;
    owner.connect( s, []( int i ){ std::cout << i << std::endl; } );
    owner.connect( s, []( int i ){ std::cout << ( i + i ) << std::endl; } );
    
    s.notify( 21 ); // Prints '21' and '42'
}

s.notify( 1337 );   // Prints nothing, observer owner went out of scope
```

### Examples

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
    pg::subject<> hello_subject;
    
    auto connection = pg::connect( hello_subject, hello_function );
    
    hello_subject.notify();
    
    return 0;
}
```
The output is:
>Hello World!

So, what happened here?

First a ```hello_subject``` is declared which will be used to emit notifications to observers that are connected.

Then a connection between ```hello_subject``` and ```hello_function``` is made and stored in ```connection```.
The ```connection``` variable manages the lifetime of the connection between subject and the connected function.
The lifetime of connection ends when ```connection``` goes out of scope or gets deleted.

When ```hello_subject``` does a ```notify``` then all observer functions that are connected are called.

### Example 2

This example shows three more features of the library:
* Usage of a ```pg::connection_owner```.
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
