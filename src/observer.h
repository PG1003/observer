// MIT License
//
// Copyright (c) 2020 PG1003
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <vector>
#include <algorithm>

#ifdef __has_cpp_attribute
# if __has_cpp_attribute( nodiscard )
#  define PG_OBSERVER_NODISCARD [[nodiscard]]
# endif
# if __has_cpp_attribute( likely )
#  define PG_OBSERVER_LIKELY [[likely]]
# endif
#endif
#ifndef PG_OBSERVER_NODISCARD
# define PG_OBSERVER_NODISCARD
#endif
#ifndef PG_OBSERVER_LIKELY
# define PG_OBSERVER_LIKELY
#endif

namespace pg
{

namespace detail
{

// This base class is to type erase the observers so that different observer types can be collected in a container.
class apex_observer
{
public:
    virtual ~apex_observer() noexcept = default;
};

}

/**
 * \brief Interface for observer objects.
 *
 * \tparam A Defines the types of the parameters for the observer's notify function.
 *
 * \see subject
 */
template< typename ...A >
class observer : public detail::apex_observer
{
public:
    /**
     * \brief Called when the observer is disconnected from the subject.
     *
     * The subject calls disconnect on all its observers when the subject gets deleted or goes out of scope.
     */
    virtual void disconnect() noexcept = 0;

    /**
     *  \brief The notification function that is called when the subject notifies
     *
     *  \param args The values of the notification. These are defined by this class' template parameters.
     */
    virtual void notify( A... args ) = 0;
};

namespace detail
{

// https://en.cppreference.com/w/cpp/types/remove_reference
template< typename T > struct remove_reference        { typedef T type; };
template< typename T > struct remove_reference< T & > { typedef T type; };
template< typename T > struct remove_reference< T && >{ typedef T type; };

// https://stackoverflow.com/a/27867127
template< typename T >
struct invoke_helper : invoke_helper< decltype( &detail::remove_reference< T >::type::operator() ) > {};

template< typename R, typename ...A >
struct invoke_helper< R ( * )( A... ) >
{
    template< typename F, typename ...Ar >
    static decltype( auto ) invoke( F&& function, A... args, Ar&&... )
    {
        return function( std::forward< A >( args )... );
    }
};

template< typename R, typename C, typename ...A >
struct invoke_helper< R ( C:: * )( A... ) >
{
    template< typename F, typename ...Ar >
    static decltype( auto ) invoke( F&& function, A... args, Ar&&... )
    {
        return function( std::forward< A >( args )... );
    }
};

template< typename R, typename C, typename ...A >
struct invoke_helper< R ( C:: * )( A... ) const >
{
    template< typename F, typename ...Ar >
    static decltype( auto ) invoke( F&& function, A... args, Ar&&... )
    {
        return function( std::forward< A >( args )... );
    }
};

template< typename ...A >
class subject_base
{
    subject_base( const subject_base< A... > & ) = delete;
    subject_base< A... >& operator=( const subject_base< A... > & ) = delete;

protected:
    std::vector< observer< A... > * > m_observers;

    subject_base() noexcept = default;

    ~subject_base() noexcept
    {
        for( auto it = m_observers.rbegin() ; it != m_observers.crend() ; ++it )
        {
            ( *it )->disconnect();
        }
    }

public:
    void connect( observer< A... > * const o ) noexcept
    {
        m_observers.push_back( o );
    }

    void disconnect( const observer< A... > * const o ) noexcept
    {
        // Iterate reversed over the m_observers since we expect that observers that
        // are frequently connected and disconnected resides at the end of the vector.
        auto it_find = std::find( m_observers.crbegin(), m_observers.crend(), o );
        if( it_find != m_observers.crend() ) PG_OBSERVER_LIKELY
        {
            m_observers.erase( ( ++it_find ).base() );
        }
    }
};

}

/**
 * \brief Invokes a callable with the given arguments.
 *
 * \param function A callable such as a free function, lambda, std::function or a functor.
 * \param args     The arguments to forward to \em function.
 *
 * \return Returns the value that the invoked function returned.
 *
 * The advantage this implementation over std::invoke is that the number of parameters that is
 * forwarded to \em function is adjusted when \em function accept less parameters than passed to \em invoke.
 */
template< typename F, typename ...A >
inline decltype( auto ) invoke( F&& function, A&&... args )
{
    return detail::invoke_helper< F >::invoke( std::forward< F >( function ), std::forward< A >( args )... );
}

/**
 * \overload inline decltype( auto ) invoke( F&& function, A&&... args )
 */
template< typename R, typename ...Af, typename ...A >
inline decltype( auto ) invoke( R ( * function )( Af... ), A&&... args )
{
    return detail::invoke_helper< R ( * )( Af... ) >::invoke( std::forward< R ( * )( Af... ) >( function ), std::forward< A >( args )... );
}

/**
 * \brief Class that calls the notification function of its observers.
 *
 * \tparam A The types of the values that are passed to the observers notification functions.
 *
 * \see observer
 */
template< typename ...A >
class subject : public detail::subject_base< A... >
{
    subject( const subject< A... > & ) = delete;
    subject< A... >& operator=( const subject< A... > & ) = delete;

public:
    subject() noexcept = default;

    /**
     * \brief Notifies the observers observers connected to this subject.
     *
     * \param args The values passed to the observer's notification function.
     *
     * The observers are notified in the order they are connected.
     */
    void notify( A... args ) const
    {
        for( auto o : detail::subject_base< A... >::m_observers )
        {
            o->notify( args... );
        }
    }
};

/**
 * \brief Class that calls the notification function of its observers.
 *
 * \tparam A The types of the values that are passed to the observers notification functions.
 *
 * This subject type is blockable.
 * It maintains a block count that is tested before the notification of its observers.
 * This subject blocks notifications when the block count is non-zero.
 *
 * \see observer
 */
template< typename ...A >
class blockable_subject : public detail::subject_base< A... >
{
    blockable_subject( const blockable_subject< A... > & ) = delete;
    blockable_subject< A... >& operator=( const blockable_subject< A... > & ) = delete;

    int block_count = 0;

public:
    blockable_subject() noexcept = default;

    /**
     * \brief Notifies the observers observers connected to this subject when not blocked.
     *
     * \param args The values passed to the observer's notification function.
     *
     * The observers are notified in the order they are connected.
     */
    void notify( A... args ) const
    {
        if( !block_count )
        {
            for( auto o : detail::subject_base< A... >::m_observers )
            {
                o->notify( args... );
            }
        }
    }

    /**
     * \brief Blocks the notification of its observers when called.
     * 
     * This function can be called multiple times which increases a block count.
     * You must call \em unblock the same number of times to unblock the subject or call \em set_block_state( false ).
     *
     * Notifications are not buffered when the subject is blocked.
     *
     * \see blockable_subject::unblock blockable_subject::set_block_state
     */
    void block() noexcept
    {
        ++block_count;
    }

    /**
     * \brief Decreases the block count and unblocks the subject when the count got 0.
     *
     * This function can be called multiple times even when the subject is already unblocked.
     *
     * \see blockable_subject::block blockable_subject::set_block_state
     */
    void unblock() noexcept
    {
        if( block_count > 0 )
        {
            --block_count;
        }
    }

    /**
     * \brief Overrides the block count when the state differs from the current block state.
     *
     * \param block_state The new block state.
     *
     * \return Returns the old block state.
     *
     * When the block count is 0 when the block_state is true, the block count is set to 1.
     * In case the block count is larger than 0 and the block_state is false, the block count is set to 0.
     *
     * The block count won't change if block_state is true and the block count is 0 or the block_state is false and the block count is 0.
     *
     * \see blockable_subject::block blockable_subject::unblock
     */
    bool set_block_state( bool block_state ) noexcept
    {
        if( block_count && !block_state )
        {
            block_count = 0;
            return true;
        }
        else if( !block_count && block_state )
        {
            block_count = 1;
            return false;
        }

        return block_state; // No state change
    }
};

/**
 * \brief Blocks temporary the notifications of a subject.
 *
 * \tparam S The type of the blockable subject.
 *
 * This class use RAII. The class blocks notifications when it is constructed and unblocked at destruction,
 * for example when a subject_blocker instance leaves its scope.
 *
 * A subject_blocker expects a blockable subject that has the following two methods;
 * - [discarded] block() [const]
 * - [discarded] unblock() [const]
 *
 * \see pg::blockable_subject
 */
template< typename S >
class subject_blocker
{
    S * m_subject = nullptr;

public:
    subject_blocker() noexcept = default;

    /**
     * \param subject A reference to the subject which notifications must be blocked.
     */
    subject_blocker( S & subject ) noexcept
            : m_subject( &subject )
    {
        m_subject->block();
    }

    ~subject_blocker() noexcept
    {
        if( m_subject )
        {
            m_subject->unblock();
        }
    }
};

namespace detail
{

template< template< class, class, class... > class D, typename B, typename S >
struct observer_type_factory : observer_type_factory< D, B, decltype( &S::connect ) >
{};

template< template< class, class, class... > class D, typename B, typename R, typename S, typename ...Ao >
struct observer_type_factory< D, B, R ( S:: * )( observer< Ao... > * ) noexcept >
{
    using type = D< B, S, Ao... >;
};

template< template< class, class, class... > class D, typename B, typename R, typename S, typename ...Ao >
struct observer_type_factory< D, B, R ( S:: * )( observer< Ao... > * ) const noexcept >
{
    using type = D< B, S, Ao... >;
};

template< typename O, typename F, typename ...Ao >
class member_function_observer
{
    O * const m_instance;
    F         m_function;

protected:
    member_function_observer( O * instance, F function ) noexcept
            : m_instance( instance )
            , m_function( function )
    {}

    template< typename ...Ar >
    void invoke( Ao&&... args, Ar&&... )
    {
        ( m_instance->*m_function )( std::forward< Ao >( args )... );
    }
};

template< typename F >
class function_observer
{
    F m_function;

protected:
    function_observer( F&& function ) noexcept
            : m_function( std::forward< F >( function ) )
    {}

    template< typename ...As >
    void invoke( As&&... args )
    {
        detail::invoke_helper< F >::invoke( m_function, std::forward< As >( args )... );
    }
};

}

/**
 * \brief Manages the subject <--> observer connection lifetime from the observer side.
 *
 * Connections that are made and thus owned by connection_owner are removed at destruction of connection_owner.
 *
 * When connecting an observer to a subject, the connection_owner expects that a subject has the following methods;
 * - [discarded] connect( pg::observer< T... > * ) [const]
 * - [discarded] disconnect( [const] pg::observer< T... > * ) [const]
 *
 * \see pg::subject pg::blockable_subject pg::observer
 */
class connection_owner
{
    class abstract_observer
    {
    public:
        virtual ~abstract_observer() noexcept = default;
        virtual void remove_from_subject() noexcept = 0;
    };

    template< typename B, typename S, typename ...Ao >
    class owner_observer final : public observer< Ao... >, public abstract_observer, B
    {
        connection_owner &m_owner;
        S                &m_subject;

        virtual void notify( Ao... args ) override
        {
            B::invoke( std::forward< Ao >( args )... );
        }

        virtual void disconnect() noexcept override
        {
            m_owner.remove_observer( this );
        }

        virtual void remove_from_subject() noexcept override
        {
            m_subject.disconnect( this );
        }

    public:
        template< typename ...Ab >
        owner_observer( connection_owner &owner, S &subject, Ab&&... args_base ) noexcept
                : B( std::forward< Ab >( args_base )... )
                , m_owner( owner )
                , m_subject( subject )
        {
            m_owner.add_observer( this );
            m_subject.connect( this );
        }
    };

    connection_owner( const connection_owner & ) = delete;
    connection_owner & operator=( const connection_owner & ) = delete;

    std::vector< abstract_observer * > m_observers;

    auto find_observer( const abstract_observer * const o )
    {
        auto it_find = std::find( m_observers.crbegin(), m_observers.crend(), o );

        return it_find == m_observers.crend() ? m_observers.cend() : ( ++it_find ).base();
    }

    void remove_observer( abstract_observer * const o ) noexcept
    {
        auto it_find = find_observer( o );
        if( it_find != m_observers.cend() ) PG_OBSERVER_LIKELY
        {
            m_observers.erase( it_find );
            delete o;
        }
    }

    void add_observer( abstract_observer * const o ) noexcept
    {
        m_observers.push_back( o );
    }

public:
    /**
     * \brief A handle to a subject <--> observer connection.
     *
     * \note Reuse of this handle may lead to undefined behavior.
     *
     * \see pg::connection_owner::connect pg::connection_owner::disconnect
     */
    class connection
    {
        friend connection_owner;
        abstract_observer * m_h = nullptr;

        connection( abstract_observer * h )
                : m_h( h )
        {}

    public:
        connection() noexcept = default;
    };

    connection_owner() = default;

    ~connection_owner() noexcept
    {
        for( auto it = m_observers.crbegin() ; it != m_observers.crend() ; ++it )
        {
            ( *it )->remove_from_subject();
            delete *it;
        }
    }

    /**
     * \brief Connects a member function of an object to a subject.
     *
     * \param s        The subject from which the object's member function will receive notifications.
     * \param instance The instance of the object.
     * \param function The member function pointer that is called when the subject notifies
     *
     * \return Returns a connection_owner::connection handle.
     *
     * The number of parameters that \em function accepts can be less than the number of values that comes with the notification.
     *
     * \note The lifetime of the instance must exceed the connection_owner's lifetime.
     */
    template< typename S, typename R, typename O, typename ...Ao >
    connection connect( S &s, O * instance, R ( O::* const function )( Ao... ) ) noexcept
    {
        using observer_type = typename detail::observer_type_factory< owner_observer, detail::member_function_observer< O, R ( O::* )( Ao... ), Ao... >, S >::type;
        return new observer_type( *this, s, instance, function );
    }

    /**
     * \overload connect( S & s, O * instance, R( O::* )( Ao... ) function )
     */
    template< typename S, typename R, typename O, typename ...Ao >
    connection connect( S &s, O * instance, R ( O::* const function )( Ao... ) const ) noexcept
    {
        using observer_type = typename detail::observer_type_factory< owner_observer, detail::member_function_observer< O, R ( O::* )( Ao... ) const, Ao... >, S >::type;
        return new observer_type( *this, s, instance, function );
    }

    /**
     * \overload connect( S & s, O * instance, R( O::* )( Ao... ) function )
     */
    template< typename S, typename R, typename O, typename ...Ao >
    connection connect( S &s, const O * instance, R ( O::* const function )( Ao... ) const ) noexcept
    {
        using observer_type = typename detail::observer_type_factory< owner_observer, detail::member_function_observer< const O, R ( O::* )( Ao... ) const, Ao... >, S >::type;
        return new observer_type( *this, s, instance, function );
    }

    /**
     * \brief Connects a callable to a subject.
     *
     * \param s        The subject from which \em function will receive notifications.
     * \param function A callable such as a free function, lambda, std::function or a functor that is called when the subject notifies.
     *
     * \return Returns a connection_owner::connection handle.
     *
     * The number of parameters that \em function accepts can be less than the number of values that comes with the notification.
     *
     * \note The \em function is copied and stored in the connection_owner. This means that the callables must have a copy constructor.
     *
     * \note When a callable has side effects than the lifetime of these side effects must exceed the connection_owner's lifetime.
     */
    template< typename S, typename F >
    connection connect( S &s, F&& function ) noexcept
    {
        using observer_type = typename detail::observer_type_factory< owner_observer, detail::function_observer< F >, S >::type;
        return new observer_type( *this, s, std::forward< F >( function ) );
    }

    /**
     * \overload connection connect( S &s, F function ) noexcept
     */
    template< typename S, typename R, typename ...Af >
    connection connect( S &s, R ( * function )( Af... ) ) noexcept
    {
        using observer_type = typename detail::observer_type_factory< owner_observer, detail::function_observer< R ( * )( Af... ) >, S >::type;
        return new observer_type( *this, s, std::forward< R ( * )( Af... ) >( function ) );
    }

    /**
     * \brief Disconnects the observer from its subject.
     *
     * \param c The connection handle.
     *
     * The observer will be deleted in case it is a lambda, std::function or a functor.
     *
     * \see pg::connection_owner::connect
     */
    void disconnect( connection c ) noexcept
    {
        auto it_find = find_observer( c.m_h );
        if( it_find != m_observers.cend() ) PG_OBSERVER_LIKELY
        {
            c.m_h->remove_from_subject();
            m_observers.erase( it_find );
            delete c.m_h;
        }
    }
};

/**
 * \brief Defined for backwards compatibility.
 *
 * observer_owner was renamed to connection_owner.
 * The name 'connection_owner' expresses better what the object does; owning the lifetime of the connections.
 */
using observer_owner [[deprecated]] = connection_owner;

namespace detail
{

template< typename B, typename S, typename ...Ao >
class scoped_observer final : public observer< Ao... >, B
{
    S * m_subject;

    virtual void notify( Ao... args ) override
    {
        B::invoke( std::forward< Ao >( args )... );
    }

    virtual void disconnect() noexcept override
    {
        m_subject = nullptr;
    }

public:
    template< typename ...Ab >
    scoped_observer( S &subject, Ab&&... args_base ) noexcept
            : B( std::forward< Ab >( args_base )... )
            , m_subject( &subject )
    {
        m_subject->connect( this );
    }

    virtual ~scoped_observer()
    {
        if( m_subject )
        {
            m_subject->disconnect( this );
        }
    }
};

}

/**
 * \brief Owns a connection between subject <--> observer.
 *
 * A pg::scoped_connection is created by the pg::connect functions.
 * The lifetime of the connection owned by pg::scoped_connection and ends when a pg::scoped_connection goes out of scope or gets deleted.
 *
 * When assigning a new connection returned by pg::connect, the connection it currently holds will be deleted before taking ownership of the new one.
 *
 * \note The lifetime of a scoped_connection must exceed the connected (member) function's side-effects' lifetime at the observer side.
 *
 * \see pg::connect
 */
class scoped_connection
{
    template< typename S, typename R, typename O, typename ...Ao >
    friend scoped_connection connect( S &s, O * instance, R ( O::* const function )( Ao... ) ) noexcept;

    template< typename S, typename R, typename O, typename ...Ao >
    friend scoped_connection connect( S &s, O * instance, R ( O::* const function )( Ao... ) const ) noexcept;

    template< typename S, typename R, typename O, typename ...Ao >
    friend scoped_connection connect( S &s, const O * instance, R ( O::* const function )( Ao... ) const ) noexcept;

    template< typename S, typename R, typename ...Af >
    friend scoped_connection connect( S &s, R ( * function )( Af... ) ) noexcept;

    template< typename S, typename F >
    friend scoped_connection connect( S &s, F&& function ) noexcept;

    detail::apex_observer* m_observer = nullptr;

    scoped_connection( detail::apex_observer * o ) noexcept
            : m_observer( o )
    {}

public:
    scoped_connection() = default;

    scoped_connection( scoped_connection&& other ) noexcept
    {
        delete m_observer;
        m_observer       = other.m_observer;
        other.m_observer = nullptr;
    }

    scoped_connection& operator=( scoped_connection&& other ) noexcept
    {
        delete m_observer;
        m_observer       = other.m_observer;
        other.m_observer = nullptr;
        return *this;
    }

    ~scoped_connection() noexcept
    {
        delete m_observer;
    }

    /**
     * \brief Ends the lifetime its connection.
     */
    void reset() noexcept
    {
        delete m_observer;
        m_observer = nullptr;
    }
};

/**
 * \brief Connects a member function of an object to a subject.
 *
 * \param s        The subject from which the object's member function will receive notifications.
 * \param instance The instance of the object.
 * \param function The member function pointer that is called when the subject notifies
 *
 * \return Returns a pg::scoped_connection.
 *
 * The number of parameters that \em function accepts can be less than the number of values that comes with the notification.
 *
 * \note The lifetime of the instance must exceed the scoped_connection's lifetime.
 */
template< typename S, typename R, typename O, typename ...Ao >
PG_OBSERVER_NODISCARD scoped_connection connect( S &s, O * instance, R ( O::* const function )( Ao... ) ) noexcept
{
    using observer_type = typename detail::observer_type_factory< detail::scoped_observer, detail::member_function_observer< O, R ( O::* )( Ao... ), Ao... >, S >::type;
    return new observer_type( s, instance, function );
}

/**
 * \overload connect( S & s, O * instance, R( O::* )( Ao... ) function )
 */
template< typename S, typename R, typename O, typename ...Ao >
PG_OBSERVER_NODISCARD scoped_connection connect( S &s, O * instance, R ( O::* const function )( Ao... ) const ) noexcept
{
    using observer_type = typename detail::observer_type_factory< detail::scoped_observer, detail::member_function_observer< O, R ( O::* )( Ao... ) const, Ao... >, S >::type;
    return new observer_type( s, instance, function );
}

/**
 * \overload connect( S & s, O * instance, R( O::* )( Ao... ) function )
 */
template< typename S, typename R, typename O, typename ...Ao >
PG_OBSERVER_NODISCARD scoped_connection connect( S &s, const O * instance, R ( O::* const function )( Ao... ) const ) noexcept
{
    using observer_type = typename detail::observer_type_factory< detail::scoped_observer, detail::member_function_observer< const O, R ( O::* )( Ao... ) const, Ao... >, S >::type;
    return new observer_type( s, instance, function );
}

/**
 * \brief Connects a callable to a subject.
 *
 * \param s        The subject from which \em function will receive notifications.
 * \param function A callable such as a free function, lambda, std::function or a functor that is called when the subject notifies.
 *
 * \return Returns a pg::scoped_connection.
 *
 * The number of parameters that \em function accepts can be less than the number of values that comes with the notification.
 *
 * \note The \em function is copied and stored in the connection_owner. This means that the callables must have a copy constructor.
 *
 * \note When a callable has side effects than the lifetime of these side effects must exceed the scoped_connection's lifetime.
 */
template< typename S, typename F >
PG_OBSERVER_NODISCARD scoped_connection connect( S &s, F&& function ) noexcept
{
    using observer_type = typename detail::observer_type_factory< detail::scoped_observer, detail::function_observer< F >, S >::type;
    return new observer_type( s, std::forward< F >( function ) );
}

/**
 * \overload scoped_connection connect( S &s, F&& function )
 */
template< typename S, typename R, typename ...Af >
PG_OBSERVER_NODISCARD scoped_connection connect( S &s, R ( * function )( Af... ) ) noexcept
{
    using observer_type = typename detail::observer_type_factory< detail::scoped_observer, detail::function_observer< R ( * )( Af... ) >, S >::type;
    return new observer_type( s, std::forward< R ( * )( Af... ) >( function ) );
}

}

