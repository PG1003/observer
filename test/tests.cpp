#include <observer.h>
#include <iostream>
#include <string>
#include <functional>
#include <memory>

static int total_asserts  = 0;
static int failed_asserts = 0;

static bool report_failed_assert( const char* const file, const int line, const char* const condition )
{
    std::cout << "assert failed! (file " << file << ", line " << line << "): " << condition << std::endl;
    ++failed_asserts;
    return false;
}

#define assert_true( c ) do { ++total_asserts; ( c ) || report_failed_assert( __FILE__, __LINE__, #c ); } while( false );


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

static int free_function_return_int()
{
    return 42;
}

struct functor_int
{
    int &val;

    functor_int( int &i )
        : val( i )
    {}

    void operator()( int i )
    {
        val = i;
    }
};

struct functor_void
{
    int &val;

    functor_void( int &i )
        : val( i )
    {}

    void operator()()
    {
        ++val;
    }
};

struct functor_return_int
{
    int operator()( int i )
    {
        return i;
    }
};

struct const_functor_return_int
{
    int operator()( int i ) const
    {
        return i;
    }
};

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
    subject<>      subject_void;

    owner.connect( subject_int, free_function_int );
    owner.connect( subject_int, free_function_void );
    owner.connect( subject_void, free_function_void );

    subject_int.notify( 42 );
    assert_true( free_function_int_val == 42 );
    assert_true( free_function_void_val == 1 );

    subject_void.notify();
    assert_true( free_function_void_val == 2 );
}

static void lambda_function_observer()
{
    observer_owner owner;
    subject< int > subject_int;
    subject<>      subject_void;

    int lambda_int_val  = -1;
    int lambda_void_val = 0;

    owner.connect( subject_int, [ & ]( int i ){ lambda_int_val = i; } );
    owner.connect( subject_int, [ & ]{ ++lambda_void_val; } );
    owner.connect( subject_void, [ & ]{ ++lambda_void_val; } );

    subject_int.notify( 42 );
    assert_true( lambda_int_val == 42 );
    assert_true( lambda_void_val == 1 );

    subject_void.notify();
    assert_true( lambda_void_val == 2 );
}

static void std_function_observer()
{
    observer_owner owner;
    subject< int > subject_int;
    subject<>      subject_void;

    int lambda_int_val  = -1;
    int lambda_void_val = 0;

    std::function< void( int ) > std_function_int  = [ & ]( int i ){ lambda_int_val = i; };
    std::function< void() >      std_function_void = [ & ]{ ++lambda_void_val; };

    owner.connect( subject_int, std_function_int );
    owner.connect( subject_int, std_function_void );
    owner.connect( subject_void, std_function_void );

    subject_int.notify( 1337 );
    assert_true( lambda_int_val == 1337 );
    assert_true( lambda_void_val == 1 );

    subject_void.notify();
    assert_true( lambda_void_val == 2 );
}

static void functor_observer()
{
    observer_owner owner;
    subject< int > subject_int;

    int int_val  = -1;
    int void_val = 0;

    owner.connect( subject_int, functor_int( int_val ) );
    owner.connect( subject_int, functor_void( void_val ) );

    subject_int.notify( 1003 );
    assert_true( int_val == 1003 );
    assert_true( void_val == 1 );
}

static void member_function_observer()
{
    subject< int, char >        subject_int_char;
    member_observers_with_owner member_observers_with_owner_( subject_int_char );

    subject_int_char.notify( 1337, 'Q' );
    assert_true( member_observers_with_owner_.int_char_ival == 1337 );
    assert_true( member_observers_with_owner_.int_char_cval == 'Q' );
    assert_true( member_observers_with_owner_.int_ival == 1337 );
    assert_true( member_observers_with_owner_.void_val == 1 );
}


static void subject_subject_observer()
{
    observer_owner       owner;
    subject< int, char > subject_int_char1;
    subject< int, char > subject_int_char2;
    subject< int >       subject_int;
    subject<>            subject_void;

    int  int_char_1_ival = -1;
    char int_char_1_cval = '\0';
    int  int_char_2_ival = -1;
    char int_char_2_cval = '\0';
    int  int_val         = -1;
    int  void_val        = 0;

    owner.connect( subject_int_char1, [ & ]( int i, char c ){ int_char_1_ival = i; int_char_1_cval = c; } );
    owner.connect( subject_int_char1, &subject_int_char2, &subject< int, char >::notify );
    owner.connect( subject_int_char2, [ & ]( int i, char c ){ int_char_2_ival = i; int_char_2_cval = c; } );
    owner.connect( subject_int_char2, &subject_int, &subject< int >::notify );
    owner.connect( subject_int, [ & ]( int i ){ int_val = i; } );
    owner.connect( subject_int, &subject_void, &subject<>::notify );
    owner.connect( subject_void, [ & ]{ ++void_val; } );

    subject_int_char1.notify( 33, 'R' );
    assert_true( int_char_1_ival == 33 );
    assert_true( int_char_1_cval == 'R' );
    assert_true( int_char_2_ival == 33 );
    assert_true( int_char_2_cval == 'R' );
    assert_true( int_val == 33 );
    assert_true( void_val == 1 );
}

static void observer_owner_lifetime()
{
    subject< int, char > subject_int_char;

    int val = -1;

    {
        observer_owner   owner;

        owner.connect( subject_int_char, [ & ]( int i ){ val = i; } );

        subject_int_char.notify( 1701, 'J' );
        assert_true( val == 1701 );
    }

    subject_int_char.notify( 1702, 'K' );
    assert_true( val == 1701 );
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
    assert_true( val_1 == 1 );
    assert_true( val_2 == 1 );
}

static void observer_disconnect()
{
    observer_owner owner_1;
    observer_owner owner_2;
    subject<>      subject_void;

    int val = 0;

    observer_owner::connection connection_1;
    connection_1                            = owner_1.connect( subject_void, [ & ]{ ++val; } );
    observer_owner::connection connection_2 = owner_2.connect( subject_void, [ & ]{ ++val; } );

    owner_2.disconnect( connection_1 );
    owner_1.disconnect( connection_2 );

    subject_void.notify();
    assert_true( val == 2 );

    owner_1.disconnect( connection_1 );
    owner_2.disconnect( connection_2 );

    subject_void.notify();
    assert_true( val == 2 );
}

static void observer_notify_and_disconnect_order()
{
    struct test_observer final : public observer<>
    {
        int       &m_counter;
        const int m_expected_value;

        test_observer( int &counter, int expected_value ) noexcept
                : m_counter( counter )
                , m_expected_value( expected_value )
        {}

        virtual void notify() override
        {
            ++m_counter;
            assert_true( m_counter == m_expected_value );
        }

        virtual void disconnect() noexcept override
        {
            assert_true( m_counter == m_expected_value );
            --m_counter;
        }
    };

    int shared_counter = 0;

    test_observer to_1( shared_counter, 1 );
    test_observer to_2( shared_counter, 2 );
    test_observer to_3( shared_counter, 3 );

    subject<> s;
    s.connect( &to_1 );
    s.connect( &to_2 );
    s.connect( &to_3 );

    s.notify();
}

static void block_subject()
{
    observer_owner      owner;
    blockable_subject<> subject_void;

    int val = 0;

    owner.connect( subject_void, [ & ]{ ++val; } );

    subject_void.notify();
    assert_true( val == 1 );

    {
        subject_blocker< blockable_subject<> > blocker( subject_void );

        subject_void.notify();
        assert_true( val == 1 );
    }

    subject_void.notify();
    assert_true( val == 2 );

    subject_void.block();
    subject_void.block();

    subject_void.notify();
    assert_true( val == 2 );

    subject_void.set_block_state( false );

    subject_void.notify();
    assert_true( val == 3 );

    subject_void.set_block_state( true );

    subject_void.notify();
    assert_true( val == 3 );
}

static void type_compatibility()
{
    observer_owner owner;
    subject< std::string >         subject_string;
//    subject< const std::string >   subject_const_string;
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

//    owner.connect( subject_const_string, str           );
//    owner.connect( subject_const_string, const_str_ref );

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
    assert_true( int_str == 5 );
    assert_true( int_const_string_ref == 5 );
    int_reset();

//    subject_const_string.notify( "Foobar" );
//    subject_const_string.notify( string_value );
//    subject_const_string.notify( const_string_value );
//    subject_const_string.notify( sz_char );
//    subject_const_string.notify( const_p_char_value );
//    assert_true( int_str == 5 );
//    assert_true( int_const_string_ref == 5 );
//    int_reset();

    subject_const_string_ref.notify( "Foobar" );
    subject_const_string_ref.notify( string_value );
    subject_const_string_ref.notify( const_string_value );
    subject_const_string_ref.notify( sz_char );
    subject_const_string_ref.notify( const_p_char_value );
    assert_true( int_str == 5 );
    assert_true( int_const_string_ref == 5 );
    int_reset();

    subject_p_char.notify( sz_char );
    assert_true( int_str == 1 );
    assert_true( int_const_string_ref == 1 );
    assert_true( int_p_char == 1 );
    assert_true( int_const_p_char == 1 );
    int_reset();

    subject_const_p_char.notify( "Foobar" );
    subject_const_p_char.notify( sz_char );
    subject_const_p_char.notify( const_p_char_value );
    assert_true( int_str == 3 );
    assert_true( int_const_string_ref == 3 );
    assert_true( int_const_p_char == 3 );
}


static void invoke_function()
{
    // Free functions
    free_function_reset();

    invoke( free_function_void, "pg", 1003 );
    assert_true( free_function_void_val == 1 );

    invoke( free_function_int, 42 );
    assert_true( free_function_int_val == 42 );

    const int int_free_function = invoke( free_function_return_int );
    assert_true( int_free_function == 42 );

    // Functor
    int int_functor_0  = -1;
    invoke( functor_int( int_functor_0 ), 42 );
    assert_true( int_functor_0 == 42 );

    const int int_functor_1 = invoke( functor_return_int(), 42, "foobar" );
    assert_true( int_functor_1 == 42 );

    const int int_functor_2 = invoke( functor_return_int(), 42 );
    assert_true( int_functor_2 == 42 );

    // Functor with const function
    const int int_const_functor_0 = invoke( const_functor_return_int(), 42, "foobar" );
    assert_true( int_const_functor_0 == 42 );

    const int int_const_functor_1 = invoke( const_functor_return_int(), 42 );
    assert_true( int_const_functor_1 == 42 );

    // Lambda
    const int int_lambda = invoke( []( int i ){ return i * 2; }, 21, 1337 );
    assert_true( int_lambda == 42 );

    // std::function
    int int_std_function = -1;
    const std::function< void( void ) > std_function = [&]{ int_std_function = 42; };
    pg::invoke( std_function ); // ADL kicked in... explicit use pg::invoke to avoid ambiguity with std::invoke.
    assert_true( int_std_function == 42 );
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
    observer_notify_and_disconnect_order();
    block_subject();
    type_compatibility();
    invoke_function();

    std::cout << "Total asserts: " << total_asserts << ", asserts failed: " << failed_asserts << std::endl;

    return failed_asserts ? 1 : 0;
}
