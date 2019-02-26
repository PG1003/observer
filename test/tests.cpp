#include <observer.h>
#include <iostream>
#include <string>
#include <functional>

static int total_checks  = 0;
static int failed_checks = 0;

static bool report_failed_check( const char* const file, const int line, const char* const condition )
{
    std::cout << "check failed! (file " << file << ", line " << line << "): " << condition << std::endl;
    ++failed_checks;
    return false;
}

#define check( c ) do { ++total_checks; ( c ) || report_failed_check( __FILE__, __LINE__, #c ); } while( false );


using namespace pg;

///////////////////////////////////////////////////////////////////////////////
//                Functions and structs used by the tests                    //
///////////////////////////////////////////////////////////////////////////////

static int free_function_int_val = -1;
static void free_function_int( int i )
{
    free_function_int_val = i;
}

static int free_function_void_val = 0;
static void free_function_void()
{
    ++free_function_void_val;
}

static void free_function_reset()
{
    free_function_int_val  = -1;
    free_function_void_val = 0;
}

struct member_observers
{
    int int_char_ival = -1;
    int int_char_cval = -1;
    int int_ival      = -1;
    int void_val      = 0;

    member_observers( observer_owner& owner, subject< int, char > &subject_int_char )
    {
        owner.connect( subject_int_char, this, &member_observers::int_char );
        owner.connect( subject_int_char, this, &member_observers::int_ );
        owner.connect( subject_int_char, this, &member_observers::void_ );
    }

    void int_char( int i, char c )
    {
        int_char_ival = i;
        int_char_cval = c;
    }

    void int_( int i )
    {
        int_ival = i;
    }

    void void_()
    {
        ++void_val;
    }
};

struct member_observers_with_owner : private observer_owner, public member_observers
{
    member_observers_with_owner( subject< int, char > &subject_int_char )
            : observer_owner()
            , member_observers( *this, subject_int_char )
    {}
};


///////////////////////////////////////////////////////////////////////////////
//                                  Tests                                    //
///////////////////////////////////////////////////////////////////////////////

static void free_function_observer()
{
    free_function_reset();

    observer_owner owner;
    subject< int > subject_int;
    subject<>        subject_void;

    owner.connect( subject_int, free_function_int );
    owner.connect( subject_int, free_function_void );
    owner.connect( subject_void, free_function_void );

    subject_int.notify( 42 );
    check( free_function_int_val == 42 );
    check( free_function_void_val == 1 );

    subject_void.notify();
    check( free_function_void_val == 2 );
}

static void lambda_function_observer()
{
    observer_owner owner;
    subject< int > subject_int;
    subject<>        subject_void;

    int lambda_int_val  = -1;
    int lambda_void_val = 0;

    owner.connect( subject_int, [ & ]( int i ){ lambda_int_val = i; } );
    owner.connect( subject_int, [ & ]{ ++lambda_void_val; } );
    owner.connect( subject_void, [ & ]{ ++lambda_void_val; } );

    subject_int.notify( 42 );
    check( lambda_int_val == 42 );
    check( lambda_void_val == 1 );

    subject_void.notify();
    check( lambda_void_val == 2 );
}

static void std_function_observer()
{
    observer_owner owner;
    subject< int > subject_int;
    subject<>        subject_void;

    int lambda_int_val  = -1;
    int lambda_void_val = 0;

    std::function< void( int ) > std_function_int  = [ & ]( int i ){ lambda_int_val = i; };
    std::function< void() >      std_function_void = [ & ]{ ++lambda_void_val; };

    owner.connect( subject_int, std_function_int );
    owner.connect( subject_int, std_function_void );
    owner.connect( subject_void, std_function_void );

    subject_int.notify( 1337 );
    check( lambda_int_val == 1337 );
    check( lambda_void_val == 1 );

    subject_void.notify();
    check( lambda_void_val == 2 );
}

static void functor_observer()
{
    struct callable_int
    {
        int &val;

        callable_int( int &i )
            : val( i )
        {}

        void operator()( int i )
        {
            val = i;
        }
    };

    struct callable_void
    {
        int &val;

        callable_void( int &i )
            : val( i )
        {}

        void operator()()
        {
            ++val;
        }
    };

    observer_owner owner;
    subject< int > subject_int;

    int int_val  = -1;
    int void_val = 0;

    owner.connect( subject_int, callable_int( int_val ) );
    owner.connect( subject_int, callable_void( void_val ) );

    subject_int.notify( 1003 );
    check( int_val == 1003 );
    check( void_val == 1 );
}

static void member_function_observer()
{
    subject< int, char >        subject_int_char;
    member_observers_with_owner member_observers_with_owner_( subject_int_char );

    subject_int_char.notify( 1337, 'Q' );
    check( member_observers_with_owner_.int_char_ival == 1337 );
    check( member_observers_with_owner_.int_char_cval == 'Q' );
    check( member_observers_with_owner_.int_ival == 1337 );
    check( member_observers_with_owner_.void_val == 1 );
}

static void subject_subject_observer()
{
    observer_owner       owner;
    subject< int, char > subject_int_char1;
    subject< int, char > subject_int_char2;
    subject< int >       subject_int;
    subject<>              subject_void;

    int  int_char_1_ival = -1;
    char int_char_1_cval = '\0';
    int  int_char_2_ival = -1;
    char int_char_2_cval = '\0';
    int  int_val         = -1;
    int  void_val        = 0;

    owner.connect( subject_int_char1, [ & ]( int i, char c ){ int_char_1_ival = i; int_char_1_cval = c; } );
    owner.connect( subject_int_char1, subject_int_char2 );
    owner.connect( subject_int_char2, [ & ]( int i, char c ){ int_char_2_ival = i; int_char_2_cval = c; } );
    owner.connect( subject_int_char2, subject_int );
    owner.connect( subject_int, [ & ]( int i ){ int_val = i; } );
    owner.connect( subject_int, subject_void );
    owner.connect( subject_void, [ & ]{ ++void_val; } );

    subject_int_char1.notify( 33, 'R' );
    check( int_char_1_ival == 33 );
    check( int_char_1_cval == 'R' );
    check( int_char_2_ival == 33 );
    check( int_char_2_cval == 'R' );
    check( int_val == 33 );
    check( void_val == 1 );
}

static void observer_owner_lifetime()
{
    subject< int, char > subject_int_char;

    int val = -1;

    {
        observer_owner   owner;

        owner.connect( subject_int_char, [ & ]( int i ){ val = i; } );

        subject_int_char.notify( 1701, 'J' );
        check( val == 1701 );
    }

    subject_int_char.notify( 1702, 'K' );
    check( val == 1701 );
}

static void subject_lifetime()
{
    observer_owner owner;

    int val_1 = 0;
    int val_2 = 0;

    {
        subject<> subject_void;
        owner.connect( subject_void, [ & ]{ ++val_1; } );

        subject_void.notify();
    }

    subject<> subject_void;
    owner.connect( subject_void, [ & ]{ ++val_2; } );

    subject_void.notify();
    check( val_1 == 1 );
    check( val_2 == 1 );
}

static void observer_disconnect()
{
    observer_owner owner;
    subject<>      subject_void;

    int val = 0;

    const auto handle = owner.connect( subject_void, [ & ]{ ++val; } );

    subject_void.notify();
    check( val == 1 );

    owner.disconnect( handle );

    subject_void.notify();
    check( val == 1 );
}

static void block_subject()
{
    observer_owner owner;
    subject<>      subject_void;

    int val = 0;

    owner.connect( subject_void, [ & ]{ ++val; } );

    subject_void.notify();
    check( val == 1 );

    {
        subject_blocker< subject<> > blocker( subject_void );

        subject_void.notify();
        check( val == 1 );
    }

    subject_void.notify();
    check( val == 2 );
}

static void type_compatibility()
{
    observer_owner owner;
    subject< std::string >         subject_string;
    subject< const std::string >   subject_const_string;
    subject< const std::string & > subject_const_string_ref;
    subject< char * >              subject_p_char;
    subject< const char * >        subject_const_p_char;

    int int_str              = 0;
    int int_const_string_ref = 0;
    int int_p_char           = 0;
    int int_const_p_char     = 0;

    const auto int_reset = [ & ]
    {
        int_str              = 0;
        int_const_string_ref = 0;
        int_p_char           = 0;
        int_const_p_char     = 0;
    };

    std::string       string_value        = "Foobar";
    const std::string const_string_value  = "Foobar";
    char              sz_char[]           = "Foobar";
    const char        *const_p_char_value = "Foobar";

    const auto str           = [ & ]( std::string str ){ if( str == const_string_value ) ++int_str; };
    const auto const_str_ref = [ & ]( const std::string& str ){ if( str == const_string_value ) ++int_const_string_ref; };
    const auto p_char        = [ & ]( char *str ){ if( str == const_string_value ) ++int_p_char; };
    const auto const_p_char  = [ & ]( const char *str ){ if( str == const_string_value ) ++int_const_p_char; };

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

    subject_string.notify( "Foobar" );
    subject_string.notify( string_value );
    subject_string.notify( const_string_value );
    subject_string.notify( sz_char );
    subject_string.notify( const_p_char_value );
    check( int_str == 5 );
    check( int_const_string_ref == 5 );
    int_reset();

    subject_const_string.notify( "Foobar" );
    subject_const_string.notify( string_value );
    subject_const_string.notify( const_string_value );
    subject_const_string.notify( sz_char );
    subject_const_string.notify( const_p_char_value );
    check( int_str == 5 );
    check( int_const_string_ref == 5 );
    int_reset();

    subject_const_string_ref.notify( "Foobar" );
    subject_const_string_ref.notify( string_value );
    subject_const_string_ref.notify( const_string_value );
    subject_const_string_ref.notify( sz_char );
    subject_const_string_ref.notify( const_p_char_value );
    check( int_str == 5 );
    check( int_const_string_ref == 5 );
    int_reset();

    subject_p_char.notify( sz_char );
    check( int_str == 1 );
    check( int_const_string_ref == 1 );
    check( int_p_char == 1 );
    check( int_const_p_char == 1 );
    int_reset();

    subject_const_p_char.notify( "Foobar" );
    subject_const_p_char.notify( sz_char );
    subject_const_p_char.notify( const_p_char_value );
    check( int_str == 3 );
    check( int_const_string_ref == 3 );
    check( int_const_p_char == 3 );
}

int main( int /* argc */, char * /* argv */[] )
{
    free_function_observer();
    lambda_function_observer();
    std_function_observer();
    functor_observer();
    member_function_observer();
    subject_subject_observer();
    observer_owner_lifetime();
    subject_lifetime();
    observer_disconnect();
    block_subject();
    type_compatibility();

    std::cout << "Total tests: " << total_checks << ", Tests failed: " << failed_checks << std::endl;

    return failed_checks ? 1 : 0;
}
