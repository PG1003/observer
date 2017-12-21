#include "observer.h"
#include <iostream>
#include <string>


using namespace pg;


///////////////////////////////////////////////////////////////////////////////
//               Functions and structs used by the examples                  //
///////////////////////////////////////////////////////////////////////////////

void free_function_int( int i )
{
    std::cout << "free_function_int( int ) - " << i << std::endl;
}
void free_function_void()
{
    std::cout << "free_function_void()" << std::endl;
}

struct member_observers
{
    member_observers( observer_owner& owner, subject< int, char > &subject_int_char )
    {
        owner.connect( subject_int_char, this, &member_observers::int_char );
        owner.connect( subject_int_char, this, &member_observers::int_ );
        owner.connect( subject_int_char, this, &member_observers::void_ );
    }

    void int_char( int i, char c )
    {
        std::cout << "member_observers::int_char( int, char ) - " << i << ", " << c << std::endl;
    }

    void int_( int i )
    {
        std::cout << "member_observers::int_( int ) - " << i << std::endl;
    }

    void void_()
    {
        std::cout << "member_observers::void_()" << std::endl;
    }
};

struct member_observers_with_owner : private observer_owner, public member_observers
{
    member_observers_with_owner( subject< int, char > &subject_int_char )
            : observer_owner()
            , member_observers( *this, subject_int_char )
    {}
};

struct callable_int
{
    void operator()( int i )
    {
        std::cout << "callable::operator()( int ) - " << i << std::endl;
    }
};

struct callable_void
{
    void operator()()
    {
        std::cout << "callable::operator()()" << std::endl;
    }
};


///////////////////////////////////////////////////////////////////////////////
//                                 Examples                                  //
///////////////////////////////////////////////////////////////////////////////

static void free_function_observer_example()
{
    std::cout << "--- Free function observer ---" << std::endl;

    observer_owner owner;
    subject< int > subject_int;
    subject<>      subject_void;

    owner.connect( subject_int, free_function_int );
    owner.connect( subject_int, free_function_void );
    owner.connect( subject_void, free_function_void );

    std::cout << "> subject< int >::notify( 42 )" << std::endl;
    subject_int.notify( 42 );

    std::cout << "> subject<>::notify()" << std::endl;
    subject_void.notify();
}

static void lambda_function_observer_example()
{
    std::cout << "--- Lambda function observer ---" << std::endl;

    observer_owner owner;
    subject< int > subject_int;
    subject<>      subject_void;

    owner.connect( subject_int, []( int i ){ std::cout << "lambda( int ) - " << i << std::endl; } );
    owner.connect( subject_int, []{ std::cout << "lambda()" << std::endl; } );
    owner.connect( subject_void, []{ std::cout << "lambda()" << std::endl; } );

    std::cout << "> subject< int >::notify( 42 )" << std::endl;
    subject_int.notify( 42 );

    std::cout << "> subject<>::notify()" << std::endl;
    subject_void.notify();
}

static void std_function_observer_example()
{
    std::cout << "--- std::function observer ---" << std::endl;

    observer_owner owner;
    subject< int > subject_int;
    subject<>      subject_void;

    std::function< void( int ) > std_function_int;
    std::function< void() >      std_function_void;

    std_function_int  = []( int i ){ std::cout << "std::function< void( int ) > - " << i << std::endl; };
    std_function_void = callable_void();

    owner.connect( subject_int, std_function_int );
    owner.connect( subject_int, std_function_void );
    owner.connect( subject_void, std_function_void );

    std::cout << "> subject< int >::notify( 30284 )" << std::endl;
    subject_int.notify( 30284 );

    std::cout << "> subject<>::notify()" << std::endl;
    subject_void.notify();
}

static void functor_observer_example()
{
    std::cout << "--- Functor observer ---" << std::endl;

    observer_owner owner;
    subject< int > subject_int;

    callable_int  functor_observer_int;
    callable_void functor_observer_void;

    owner.connect( subject_int, functor_observer_int );
    owner.connect( subject_int, functor_observer_void );

    std::cout << "> subject< int >::notify( 1003 )" << std::endl;
    subject_int.notify( 1003 );

    std::cout << "> subject< int >::notify( 1003 )" << std::endl;
    subject_int.notify( 1003 );
}

static void member_function_observer_example()
{
    std::cout << "--- Member function observer ---" << std::endl;

    subject< int, char >        subject_int_char;
    member_observers_with_owner member_observers_with_owner_( subject_int_char );

    std::cout << "> subject< int, char >::notify( 1337, 'Q' )" << std::endl;
    subject_int_char.notify( 1337, 'Q' );
}

static void subject_subject_observer_example()
{
    std::cout << "--- Subject subject observer ---" << std::endl;

    observer_owner       owner;
    subject< int, char > subject_int_char1;
    subject< int, char > subject_int_char2;
    subject< int >       subject_int;
    subject<>            subject_void;

    owner.connect( subject_int_char1, []( int i, char c ){ std::cout << "lambda( int, char ) @ subject_int_char1 - " << i << ", " << c << std::endl; } );
    owner.connect( subject_int_char1, subject_int_char2 );
    owner.connect( subject_int_char2, []( int i, char c ){ std::cout << "lambda( int, char ) @ subject_int_char2 - " << i << ", " << c << std::endl; } );
    owner.connect( subject_int_char2, subject_int );
    owner.connect( subject_int, []( int i ){ std::cout << "lambda( int ) @ subject_int - " << i << std::endl; } );
    owner.connect( subject_int, subject_void );
    owner.connect( subject_void, []{ std::cout << "lambda() @ subject_void" << std::endl; } );

    std::cout << "> subject< int, char >::notify( 33, 'R' ) (subject_int_char1)" << std::endl;
    subject_int_char1.notify( 33, 'R' );
}

static void observer_owner_lifetime_example()
{
    std::cout << "--- Observer owner lifetime ---" << std::endl;

    subject< int, char > subject_int_char;

    {
        observer_owner   owner;
        member_observers member_observers_( owner, subject_int_char );

        std::cout << "> subject< int, char >::notify( 1701, 'J' )" << std::endl;
        subject_int_char.notify( 1701, 'J' );
    }

    std::cout << "> subject< int, char >::notify( 1701, 'J' )" << std::endl;
    subject_int_char.notify( 1701, 'J' );
}

static void subject_lifetime_example()
{
    std::cout << "--- Subject lifetime ---" << std::endl;
    observer_owner owner;

    {
        subject<> subject_void;

        owner.connect( subject_void, free_function_void );

        std::cout << "> subject<>::notify()" << std::endl;
        subject_void.notify();
    }
}

static void observer_disconnect_example()
{
    std::cout << "--- Observer disconnect ---" << std::endl;

    observer_owner owner;
    subject<>      subject_void;

    const auto handle = owner.connect( subject_void, free_function_void );

    std::cout << "> subject<>::notify()" << std::endl;
    subject_void.notify();

    owner.disconnect( handle );

    std::cout << "> subject<>::notify()" << std::endl;
    subject_void.notify();
}

static void subject_blocker_example()
{
    std::cout << "--- Subject blocker ---" << std::endl;

    observer_owner owner;
    subject<>      subject_void;

    owner.connect( subject_void, free_function_void );

    std::cout << "> subject<>::notify()" << std::endl;
    subject_void.notify();

    {
        subject_blocker< subject<> > blocker( subject_void );

        std::cout << "> subject<>::notify()" << std::endl;
        subject_void.notify();
    }

    std::cout << "> subject<>::notify()" << std::endl;
    subject_void.notify();
}

static void type_compatibility_example()
{
    std::cout << "--- Type compatibility example---" << std::endl;

    observer_owner owner;
    subject< std::string >         subject_string;
    subject< const std::string >   subject_const_string;
    subject< const std::string & > subject_const_string_ref;
    subject< char * >              subject_p_char;
    subject< const char * >        subject_const_p_char;

    const auto str           = []( std::string str ){ std::cout << "lambda( std::string ) - " << str << std::endl; };
    const auto const_str_ref = []( const std::string& str ){ std::cout << "lambda( const std::string & ) - " << str << std::endl; };
    const auto p_char        = []( char *str ){ std::cout << "lambda( char * ) - " << str << std::endl; };
    const auto const_p_char  = []( const char *str ){ std::cout << "lambda( const char * ) - " << str << std::endl; };

    owner.connect( subject_string, str           );
    owner.connect( subject_string, const_str_ref );

    owner.connect( subject_const_string, str           );
    owner.connect( subject_const_string, const_str_ref );

    owner.connect( subject_const_string_ref, str           );
    owner.connect( subject_const_string_ref, const_str_ref );

    owner.connect( subject_p_char, str           );
    owner.connect( subject_p_char, const_str_ref );
    owner.connect( subject_p_char, p_char        );
    owner.connect( subject_p_char, const_p_char  );

    owner.connect( subject_const_p_char, str           );
    owner.connect( subject_const_p_char, const_str_ref );
    owner.connect( subject_const_p_char, const_p_char  );

    std::string       string_value        = "Bar";
    const std::string const_string_value  = "Baz";
    char              sz_char[]           = "Blah";
    const char        *const_p_char_value = "Bhoo";

    std::cout << "> subject< std::string >::notify()" << std::endl;
    subject_string.notify( "Foo" );
    subject_string.notify( string_value );
    subject_string.notify( const_string_value );
    subject_string.notify( sz_char );
    subject_string.notify( const_p_char_value );

    std::cout << "> subject< const std::string >::notify()" << std::endl;
    subject_const_string.notify( "Foo" );
    subject_const_string.notify( string_value );
    subject_const_string.notify( const_string_value );
    subject_const_string.notify( sz_char );
    subject_const_string.notify( const_p_char_value );

    std::cout << "> subject< const std::string & >::notify()" << std::endl;
    subject_const_string_ref.notify( "Foo" );
    subject_const_string_ref.notify( string_value );
    subject_const_string_ref.notify( const_string_value );
    subject_const_string_ref.notify( sz_char );
    subject_const_string_ref.notify( const_p_char_value );

    std::cout << "> subject< char * >::notify()" << std::endl;
    subject_p_char.notify( sz_char );

    std::cout << "> subject< const char * >::notify()" << std::endl;
    subject_const_p_char.notify( "Foo"              );
    subject_const_p_char.notify( sz_char            );
    subject_const_p_char.notify( const_p_char_value );
}

int main( int /* argc */, char * /* argv */[] )
{
    free_function_observer_example();
    lambda_function_observer_example();
    std_function_observer_example();
    functor_observer_example();
    member_function_observer_example();
    subject_subject_observer_example();
    observer_owner_lifetime_example();
    subject_lifetime_example();
    observer_disconnect_example();
    subject_blocker_example();
    type_compatibility_example();

    return 0;
}
