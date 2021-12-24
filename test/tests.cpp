#include <observer.h>
#include <iostream>
#include <string>
#if __cplusplus >= 201703L
#include <string_view>
#endif
#include <vector>
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

struct member_observers_with_owner : private connection_owner, public member_observers
{
    member_observers_with_owner( subject< int, char > &subject_int_char )
    {
        connect( subject_int_char, static_cast< member_observers * >( this ), &member_observers::int_char );
        connect( subject_int_char, static_cast< member_observers * >( this ), &member_observers::int_ );
        connect( subject_int_char, static_cast< member_observers * >( this ), &member_observers::void_ );
    }
};

///////////////////////////////////////////////////////////////////////////////
//                                  Tests                                    //
///////////////////////////////////////////////////////////////////////////////

static void free_function_observer()
{
    free_function_reset();

    subject< int > subject_int;
    subject<>      subject_void;

    {
        connection_owner owner;

        owner.connect( subject_int, free_function_int );
        owner.connect( subject_int, free_function_void );
        owner.connect( subject_void, free_function_void );

        subject_int.notify( 42 );
        assert_true( free_function_int_val == 42 );
        assert_true( free_function_void_val == 1 );

        subject_void.notify();
        assert_true( free_function_void_val == 2 );
    }

    free_function_reset();

    {
        auto c1 = connect( subject_int, free_function_int );
        auto c2 = connect( subject_int, free_function_void );
        auto c3 = connect( subject_void, free_function_void );

        subject_int.notify( 42 );
        assert_true( free_function_int_val == 42 );
        assert_true( free_function_void_val == 1 );

        subject_void.notify();
        assert_true( free_function_void_val == 2 );
    }
}

static void lambda_function_observer()
{
    subject< int > subject_int;
    subject<>      subject_void;

    int lambda_int_val  = -1;
    int lambda_void_val = 0;

    {
        connection_owner owner;

        owner.connect( subject_int, [ & ]( int i ){ lambda_int_val = i; } );
        owner.connect( subject_int, [ & ]{ ++lambda_void_val; } );
        owner.connect( subject_void, [ & ]{ ++lambda_void_val; } );

        subject_int.notify( 42 );
        assert_true( lambda_int_val == 42 );
        assert_true( lambda_void_val == 1 );

        subject_void.notify();
        assert_true( lambda_void_val == 2 );
    }

    lambda_int_val  = -1;
    lambda_void_val = 0;

    {
        auto c1 = connect( subject_int, [ & ]( int i ){ lambda_int_val = i; } );
        auto c2 = connect( subject_int, [ & ]{ ++lambda_void_val; } );
        auto c3 = connect( subject_void, [ & ]{ ++lambda_void_val; } );

        subject_int.notify( 42 );
        assert_true( lambda_int_val == 42 );
        assert_true( lambda_void_val == 1 );

        subject_void.notify();
        assert_true( lambda_void_val == 2 );
    }
}

static void std_function_observer()
{
    subject< int > subject_int;
    subject<>      subject_void;

    int lambda_int_val  = -1;
    int lambda_void_val = 0;

    std::function< void( int ) > std_function_int  = [ & ]( int i ){ lambda_int_val = i; };
    std::function< void() >      std_function_void = [ & ]{ ++lambda_void_val; };

    {
        connection_owner owner;

        owner.connect( subject_int, std_function_int );
        owner.connect( subject_int, std_function_void );
        owner.connect( subject_void, std_function_void );

        subject_int.notify( 1337 );
        assert_true( lambda_int_val == 1337 );
        assert_true( lambda_void_val == 1 );

        subject_void.notify();
        assert_true( lambda_void_val == 2 );
    }

    lambda_int_val  = -1;
    lambda_void_val = 0;

    {
        auto c1 = connect( subject_int, std_function_int );
        auto c2 = connect( subject_int, std_function_void );
        auto c3 = connect( subject_void, std_function_void );

        subject_int.notify( 1337 );
        assert_true( lambda_int_val == 1337 );
        assert_true( lambda_void_val == 1 );

        subject_void.notify();
        assert_true( lambda_void_val == 2 );
    }
}

static void functor_observer()
{
    subject< int > subject_int;

    int int_val  = -1;
    int void_val = 0;

    {
        connection_owner owner;

        owner.connect( subject_int, functor_int( int_val ) );
        owner.connect( subject_int, functor_void( void_val ) );

        subject_int.notify( 1003 );
        assert_true( int_val == 1003 );
        assert_true( void_val == 1 );
    }

    int_val  = -1;
    void_val = 0;

    {
        auto c1 = connect( subject_int, functor_int( int_val ) );
        auto c2 = connect( subject_int, functor_void( void_val ) );

        subject_int.notify( 1003 );
        assert_true( int_val == 1003 );
        assert_true( void_val == 1 );
    }
}

static void member_function_observer()
{
    subject< int, char >        subject_int_char;

    {
        member_observers_with_owner member_observers_with_owner_( subject_int_char );

        subject_int_char.notify( 1337, 'Q' );
        assert_true( member_observers_with_owner_.int_char_ival == 1337 );
        assert_true( member_observers_with_owner_.int_char_cval == 'Q' );
        assert_true( member_observers_with_owner_.int_ival == 1337 );
        assert_true( member_observers_with_owner_.void_val == 1 );
    }

    {
        member_observers member_observers_;

        auto c1 = connect( subject_int_char, &member_observers_, &member_observers_with_owner::int_char );
        auto c2 = connect( subject_int_char, &member_observers_, &member_observers_with_owner::int_ );
        auto c3 = connect( subject_int_char, &member_observers_, &member_observers_with_owner::void_ );

        subject_int_char.notify( 1337, 'Q' );
        assert_true( member_observers_.int_char_ival == 1337 );
        assert_true( member_observers_.int_char_cval == 'Q' );
        assert_true( member_observers_.int_ival == 1337 );
        assert_true( member_observers_.void_val == 1 );
    }
}


static void subject_subject_observer()
{
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

    {
        connection_owner owner;

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

    int_char_1_ival = -1;
    int_char_1_cval = '\0';
    int_char_2_ival = -1;
    int_char_2_cval = '\0';
    int_val         = -1;
    void_val        = 0;

    {
        auto c1 = connect( subject_int_char1, [ & ]( int i, char c ){ int_char_1_ival = i; int_char_1_cval = c; } );
        auto c2 = connect( subject_int_char1, &subject_int_char2, &subject< int, char >::notify );
        auto c3 = connect( subject_int_char2, [ & ]( int i, char c ){ int_char_2_ival = i; int_char_2_cval = c; } );
        auto c4 = connect( subject_int_char2, &subject_int, &subject< int >::notify );
        auto c5 = connect( subject_int, [ & ]( int i ){ int_val = i; } );
        auto c6 = connect( subject_int, &subject_void, &subject<>::notify );
        auto c7 = connect( subject_void, [ & ]{ ++void_val; } );

        subject_int_char1.notify( 33, 'R' );
        assert_true( int_char_1_ival == 33 );
        assert_true( int_char_1_cval == 'R' );
        assert_true( int_char_2_ival == 33 );
        assert_true( int_char_2_cval == 'R' );
        assert_true( int_val == 33 );
        assert_true( void_val == 1 );
    }

}

static void observer_owner_lifetime()
{
    subject< int, char > subject_int_char;

    int val = -1;

    {
        connection_owner owner;

        owner.connect( subject_int_char, [ & ]( int i ){ val = i; } );

        subject_int_char.notify( 1701, 'J' );
        assert_true( val == 1701 );
    }

    subject_int_char.notify( 1702, 'K' );
    assert_true( val == 1701 );

    val = -1;

    {
        auto c = connect( subject_int_char, [ & ]( int i ){ val = i; } );

        subject_int_char.notify( 1701, 'J' );
        assert_true( val == 1701 );
    }

    subject_int_char.notify( 1702, 'K' );
    assert_true( val == 1701 );
}

static void subject_lifetime()
{
    int val_1 = 0;
    int val_2 = 0;

    {
        connection_owner owner;

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

    val_1 = 0;
    val_2 = 0;

    {
        scoped_connection c1;

        {
            subject<> subject_void;
            c1 = connect( subject_void, [ & ]{ ++val_1; } );

            subject_void.notify();
        }

        subject<> subject_void;
        auto c2 = connect( subject_void, [ & ]{ ++val_2; } );

        subject_void.notify();
        assert_true( val_1 == 1 );
        assert_true( val_2 == 1 );
    }
}

static void scoped_observer()
{
    scoped_connection connection;
    subject< int >    s;

    int val = 0;

    connection = connect( s, [ & ]( int i ){ val = i; } );

    s.notify( 42 );
    assert_true( val == 42 );

    {
        scoped_connection moved_connection( std::move( connection ) );

        s.notify( 1337 );
        assert_true( val == 1337 );
    }

    s.notify( 1003 );
    assert_true( val == 1337 );

    connection = connect( s, [&]{ ++val; } );

    s.notify( 42 );
    assert_true( val == 1338 );

    struct foo
    {
        int * const m_val = nullptr;

        foo( int & v )
            : m_val( &v )
        {}

        int operator()( int i )
        {
            *m_val = i * 2;
            return *m_val;
        }

        ~foo()
        {
            *m_val = 0;
        }
    };

    connection = connect( s, foo( val ) );

    s.notify( 21 );
    assert_true( val == 42 );

    connection.reset();
    assert_true( val == 0 );
}

static void observer_disconnect()
{
    connection_owner owner_1;
    connection_owner owner_2;
    subject<>        subject_void;

    int val = 0;

    connection_owner::connection connection_1;
    connection_1                              = owner_1.connect( subject_void, [ & ]{ ++val; } );
    connection_owner::connection connection_2 = owner_2.connect( subject_void, [ & ]{ ++val; } );

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
    connection_owner    owner;
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

    subject_void.unblock();

    subject_void.notify();
    assert_true( val == 4 );

    subject_void.set_block_state( true );

    subject_void.notify();
    assert_true( val == 4 );
}

static void type_compatibility()
{
    connection_owner               owner;
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
    assert_true( int_str == 5 );
    assert_true( int_const_string_ref == 5 );
    int_reset();

    subject_const_string.notify( "Foobar" );
    subject_const_string.notify( string_value );
    subject_const_string.notify( const_string_value );
    subject_const_string.notify( sz_char );
    subject_const_string.notify( const_p_char_value );
    assert_true( int_str == 5 );
    assert_true( int_const_string_ref == 5 );
    int_reset();

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

struct object_forwarding
{
    object_forwarding() = delete;
    object_forwarding( const object_forwarding & ) = delete;
    object_forwarding( const object_forwarding && ) = delete;
    object_forwarding & operator =( const object_forwarding & ) = delete;
    object_forwarding & operator =( const object_forwarding && ) = delete;

    object_forwarding( int value )
        : m_value( value )
    {}

    const int m_value;
};

void free_function_object_forwarding( const object_forwarding &o )
{
    assert_true( o.m_value == 1003 );
}

struct member_functions_object_forwarding
{
    void operator()( const object_forwarding &o )
    {
        assert_true( o.m_value == 1003 );
    }

    void foo( const object_forwarding &o )
    {
        assert_true( o.m_value == 1003 );
    }

    void bar( const object_forwarding &o ) const
    {
        assert_true( o.m_value == 1003 );
    }
};

struct const_member_functions_object_forwarding
{
    void operator()( const object_forwarding &o ) const
    {
        assert_true( o.m_value == 1003 );
    }

    void bar( const object_forwarding &o ) const
    {
        assert_true( o.m_value == 1003 );
    }
};

static void const_and_forwarding()
{
    subject< const object_forwarding & > s;
    pg::connection_owner           owner;

    member_functions_object_forwarding             member_functions;
    const member_functions_object_forwarding       member_functions_const;
    const_member_functions_object_forwarding       const_member_functions;
    const const_member_functions_object_forwarding const_member_functions_const;

    owner.connect( s, []( const object_forwarding &o )
        {
            assert_true( o.m_value == 1003 );
        } );
    owner.connect( s, free_function_object_forwarding );

    owner.connect( s, member_functions );
    owner.connect( s, &member_functions, &member_functions_object_forwarding::foo );
    owner.connect( s, &member_functions, &member_functions_object_forwarding::bar );

    owner.connect( s, &member_functions_const, &member_functions_object_forwarding::bar );

    owner.connect( s, const_member_functions );
    owner.connect( s, &const_member_functions, &const_member_functions_object_forwarding::bar );

    owner.connect( s, const_member_functions_const );
    owner.connect( s, &const_member_functions_const, &const_member_functions_object_forwarding::bar );

    const auto c1 = pg::connect( s, []( const object_forwarding &o )
        {
            assert_true( o.m_value == 1003 );
        } );
    const auto c2 = pg::connect( s, free_function_object_forwarding );

    const auto c3 = pg::connect( s, member_functions );
    const auto c4 = pg::connect( s, &member_functions, &member_functions_object_forwarding::foo );
    const auto c5 = pg::connect( s, &member_functions, &member_functions_object_forwarding::bar );

    const auto c6 = pg::connect( s, &member_functions_const, &member_functions_object_forwarding::bar );

    const auto c7 = pg::connect( s, const_member_functions );
    const auto c8 = pg::connect( s, &const_member_functions, &const_member_functions_object_forwarding::bar );

    const auto c9  = pg::connect( s, const_member_functions_const );
    const auto c10 = pg::connect( s, &const_member_functions_const, &const_member_functions_object_forwarding::bar );

    s.notify( object_forwarding( 1003 ) );
}

static std::string hello_called;
static void hello( const std::string & str )
{
    hello_called = "Hello " + str;
}

void readme_examples()
{
    // Connecting a lambda
    {
        std::string hello_world_called;

        pg::subject<> hello_subject;

        auto connection = pg::connect( hello_subject, [&]{ hello_world_called = "Hello World!"; } );

        hello_subject.notify();

        assert_true( hello_world_called == "Hello World!" );
    }

    // Connecting a function that ignores the extra parameters from subject
    {
        pg::subject< const char *, int > world_subject;

        auto connection = pg::connect( world_subject, hello );

        world_subject.notify( "World!", 42 );

        assert_true( hello_called == "Hello World!" );
    }

    // Connecting a member function
    {
        pg::subject< const std::string & > s;

        std::vector< std::string > v;

        using overload = void( std::vector< std::string >::* )( const std::string & );

        auto connection = pg::connect( s, &v, static_cast< overload >( &std::vector< std::string >::push_back ) );

        s.notify( "Hello" );
        s.notify( "World!" );

        assert_true( v.size() == 2 );
        assert_true( v[ 0 ] == "Hello" );
        assert_true( v[ 1 ] == "World!" );
    }

#if __cplusplus >= 201703L
    // Manage multiple connections using a `pg::connection_owner`
    {
        pg::subject< std::string > foo;

        std::string first_called;
        std::string second_called;

        {
            pg::connection_owner connections;

            connections.connect( foo, [&]( std::string_view message )
            {
                first_called = message.substr();
            } );

            connections.connect( foo, [&]
            {
                second_called = "Hello World!";
            } );

            foo.notify( "Hello PG1003!" );

            assert_true( first_called == "Hello PG1003!" );
            assert_true( second_called == "Hello World!" );
        }

        first_called.clear();
        second_called.clear();

        // 5 The connection owner object went out of scope and disconnected the observer functions.
        //   The next notify prints nothing.
        foo.notify( "How are you?" );

        assert_true( first_called.empty() );
        assert_true( second_called.empty() );
    }

    // Manage multiple connections by inheriting from `pg::connection_owner`
    {
        std::string print_called;
        std::string print_bar_called;

        struct bar_object : public pg::connection_owner
        {
            std::string & m_print_called;
            std::string & m_print_bar_called;

            bar_object( pg::subject< std::string > & foo, std::string & pc, std::string & pbc )
                    : m_print_called( pc )
                    , m_print_bar_called( pbc )
            {
                connect( foo, this, &bar_object::print );
                connect( foo, this, &bar_object::print_bar );
            }

            void print( std::string_view str ) { m_print_called = str; }
            void print_bar() { m_print_bar_called = "bar"; }
        };

        pg::subject< std::string > foo;

        {
            bar_object bar( foo, print_called, print_bar_called );

            foo.notify( "foo" );

            assert_true( print_called == "foo" );
            assert_true( print_bar_called == "bar" );
        }

        print_called.clear();
        print_bar_called.clear();

        foo.notify( "baz" );

        assert_true( print_called.empty() );
        assert_true( print_bar_called.empty() );
    }
#endif
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
    scoped_observer();
    observer_disconnect();
    observer_notify_and_disconnect_order();
    block_subject();
    type_compatibility();
    invoke_function();
    const_and_forwarding();
    readme_examples();

    std::cout << "Total asserts: " << total_asserts << ", asserts failed: " << failed_asserts << std::endl;

    return failed_asserts ? 1 : 0;
}
