# observer

An observer pattern that can ignore extra arguments like Qt's signals and slots.

## Features

* Connected callables can accept _less_ parameters than the subject's interface provides.
* Defining the observer's notification values types by variadic template parameters.
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

* Processing return values returned by the connected functions and observers.  
  The return value is ignored for most cases where an observer pattern or signal/slot solution is used.
  It also limit the usage of this library and adds complexity; all observers or connected functions must return the same type and the results must becombined when receiving values from multiple observers.
* Build-in mutexes/atomics/thread safety.  
  Adding these will add complexity and impact the performance for cases where these are not needed.
  When needed, you can create custom subjects that integrates with your application.
  
  When you really need these features out-of-the-box then you should try [boost signals](https://www.boost.org/doc/libs/1_72_0/doc/html/signals2.html).

## Examples

In the [examples folder](https://github.com/PG1003/observer/blob/master/examples) you will find example programs that show the features and usage of this library.
You can also take a peek in [tests.cpp](https://github.com/PG1003/observer/blob/master/test/tests.cpp).

The following examples are provided to get the impression about the usage of this observer library.

### Connecting a lambda

``` c++
int main( int /* argc */, char * /* argv */[] )
{
    // 1 Define a subject that notifies without parameters.
    pg::subject<> hello_subject;
    
    // 2 Connect a lambda to the subject.
    //   Assign the connection to a variable to keep the connection alive.
    //   The connection will be automatically removed when the variable goes out of scope.
    auto connection = pg::connect( hello_subject, []{ std::cout << "Hello World!" << std::endl; } );
    
    // 3 Notify the observers.
    //   This will call the lambda that was connected to this subject.
    //   In this case the subject has one function connected to it.
    hello_subject.notify();
    
    return 0;
}
```
The output is:
>Hello World!

### Connecting a function that ignores the extra parameters from subject

```c++
// 1 A free function that accepts one string parameter.
void hello( const std::string & str )
{
    std::cout << "Hello " << str << std::endl;
}
    
int main( int /* argc */, char * /* argv */[] )
{
    // 2 Define a subject that passes two values; a string and an integer.
    pg::subject< const char *, int > world_subject;
    
    // 4 Connect the hello function to the subject.
    //   hello takes the first string value from the subject and ignors the second integer.
    //   Assign the connection to a variable to keep the connection alive.
    auto connection = pg::connect( world_subject, hello );
    
    // 5 Notify the observers.
    //   This will call the hello function that was connected to this subject.
    world_subject.notify( "World!", 42 );
    
    return 0;
}
```
The output is:
>Hello World!

### Connecting a member function

```c++
int main( int /* argc */, char * /* argv */[] )
{
    // 1 Define a subject that passes a string to its observers.
    subject< const std::string & > s;
    
    // 2 A vector to store the values we receive from our subject.
    std::vector< std::string > v;

    // 3 Create an alias for the type of overloaded member function pointer that we want to connect.
    using overload = void( std::vector< std::string >::* )( const std::string & );
    
    // 4 Connect the vector's push_back fuction to our subject.
    //   We need to cast member function pointer to select the required overload.
    //   Assign the connection to a variable to keep the connection alive.
    auto connection = pg::connect( s, &v, static_cast< overload >( &std::vector< std::string >::push_back ) );

    // 5 Notify the observers.
    s.notify( "Hello" );
    s.notify( "World!" );

    // 6 Print the contents of the vector.
    for( auto& str : v )
    {
        std::cout << str << std::endl;
    }
    
    return 0
}
```
The output is:
>Hello  
>World!

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
pg::subject< int, const char * > // A subject that passes an integer and a string when notifying its observers.
pg::subject<>                    // A subject that notifies without values.
```

#### Custom subjects

You can create custom subjects for applications that need tight integration, multiprocessing, low overhead, etc.  

To create custom subjects you need to define the following two public member functions to connect observers and facilitate lifetime management:
* ```[discarded] connect( pg::observer< T... > * ) [const]```
* ```[discarded] disconnect( [const] pg::observer< T... > * ) [const]```

Also, you must call for each observer the ```pg::observer< T... >::disconnect``` function before removing it from the object, for example in the destructor.

The example below is a lightweight subject that handles only one observer.
Note that there is _no_ ```notify``` function.
The notify function is not a part of the static interface that defines a subject.
So you can pick any name for the notification function that calls the observer's notify like ```emit``` or build your own method to notify the observer.  

```c++
class my_subject
{
    pg::observer< my_data > * m_observer = nullptr;
    
public:    
    ~subject_base() noexcept
    {
        if( m_observer )
        {
            m_observer->disconnect();
        }
    }

    void connect( pg::observer< my_data > * const o ) noexcept
    {
        if( m_observer )
        {
            m_observer->disconnect();
        }
        m_observer = o;
    }

    void disconnect( const pg::observer< my_data > * const o ) noexcept
    {
        if( o == m_observer )
        {
            m_observer = nullptr;
        }
    }
};
```

### Connection lifetime management

Lifetime management of the connection between subjects and observers is important.
It releases resources that are no longer needed and avoids accessing resources that are no longer available.
When a subject is removed, all connections to that subject must be removed too.
The same applies when removing an observer; the observer must be disconnected from its subject. 
 
There are two methods to manage the lifetime of connections between subjects and observers:
* Scoped connection
* Connection owner

#### Scoped connections

A scoped connection is a lightweight object of the ```pg::scoped_connection``` type which owns a connection.
The lifetime of the connection ends when the lifetime of the subject ends or when the scoped connection object goes out of scope.

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
You can add a connection owner via composition by adding a ```pg::connection_owner``` member or give a object connection owner traits by deriving from it.
The lifetime of a connection is bound to the connection owner's lifetime.
Connections are automatically removed from the connection owner when the subject goes out of scope or gets deleted. 

A connection owner object owns only connections that are created with one of its ```pg::connection_owner::connect``` functions.
Connection owners do not share connections.

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
### C++17 CTAD

Although C++14 is required, C++17 introduced [CTAD](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction) which simplifies the use of ```pg::subject_blocker``` and makes defining a parameterless ```pg::subject``` prettier.

``` c++
pg::subject<> foo   // C++14
pg::subject   foo;  // C++17

pg::blockable_subject<> bar;  // C++14
pg::subject_blocker< pg::blockable_subject<> > blocker( bar );   // C++14
pg::subject_blocker                            blocker( bar );   // C++17
```
