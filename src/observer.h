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

#include <set>
#include <vector>
#include <algorithm>


namespace pg
{

/**
 * \brief Interface for observer objects.
 *
 * \tparam A Defines the types of the parameters for the observer's notify function.
 *
 * \see subject
 */
template< typename ...A >
class observer
{
public:
    virtual ~observer() noexcept = default;

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

// https://stackoverflow.com/a/27867127
template< typename T >
struct invoke_helper : invoke_helper< decltype( &T::operator() ) > {};

template< typename R, typename ...A >
struct invoke_helper< R ( * )( A... ) >
{
    template< typename F >
    static decltype( auto ) invoke( const F function, A... args, ... )
    {
        return function( std::forward< A >( args )... );
    }
};

template< typename R, typename C, typename ...A >
struct invoke_helper< R ( C:: * )( A... ) >
{
    template< typename F >
    static decltype( auto ) invoke( F function, A... args, ... )
    {
        return function( std::forward< A >( args )... );
    }
};

template< typename R, typename C, typename ...A >
struct invoke_helper< R ( C:: * )( A... ) const >
{
    template< typename F >
    static decltype( auto ) invoke( const F function, A... args, ... )
    {
        return function( std::forward< A >( args )... );
    }
};

template< typename ...A >
class subject_base
{
protected:
    std::vector< observer< A... > * > m_observers;

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
        auto it_find = std::find_if( m_observers.crbegin(), m_observers.crend(), [o]( const auto &other )
        {
            return other == o;
        } );
        if( it_find != m_observers.crend() )
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
inline decltype( auto ) invoke( const F function, A... args )
{
    return detail::invoke_helper< F >::invoke( function, std::forward< A >( args )... );
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
public:
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
    int block_count = 0;

public:
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
     * then you must call \em unblock the same number of times or \em set_block_set( false ).
     *
     * Notifications are not buffered when the subject is blocked.
     *
     * \see blockable_subject::unblock
     * \see blockable_subject::set_block_state
     */
    void block()
    {
        ++block_count;
    }

    /**
     * \brief Decreases the block count and unblocks the subject when the count got 0.
     *
     * This function can be called multiple times even when the subject is already unblocked.
     *
     * \see blockable_subject::block
     * \see blockable_subject::set_block_state
     */
    void unblock()
    {
        --block_count;
    }

    /**
     * \brief Overrides the block count when the state differs from the current block state.
     *
     * \param block_state The new block state.
     *
     * When the block count is 0 when the block_state is true, the block count is set to 1.
     * In case the block count is larger than 0 and the block_state is false, the block count is set to 0.
     *
     * The block count won't change if block_state is true and the block count is 0 or the block_state is false and the block count is 0.
     *
     * \see blockable_subject::block
     * \see blockable_subject::unblock
     */
    bool set_block_state( bool block_state )
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
 * \tparam S The type of the blocked subject.
 *
 * This class use RAII. The class blocks notifications when it is constructed and unblocked at destruction,
 * for example when a subject_blocker instance leaves its scope.
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
        if( m_subject )
        {
            m_subject->block();
        }
    }

    ~subject_blocker() noexcept
    {
        if( m_subject )
        {
            m_subject->unblock();
        }
    }
};

/**
 * \brief Manages the subject <--> observer connection lifetime from the observer side.
 *
 * Connections that are made and thus owned by observer_owner are removed at destruction of observer_owner.
 */
class observer_owner
{
    class abstract_owner_observer
    {
    public:
        virtual ~abstract_owner_observer() noexcept = default;
        virtual void remove_from_subject() noexcept = 0;
    };

    template< typename B, typename S, typename ...Ao >
    class owner_observer final : public observer< Ao... >, public abstract_owner_observer, B
    {
        observer_owner   &m_owner;
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
        owner_observer( observer_owner &owner, S &subject, Ab&&... args_base )
            : B( std::forward< Ab >( args_base )... )
            , m_owner( owner )
            , m_subject( subject )
        {
            m_owner.add_observer( this );
            m_subject.connect( this );
        }
    };

    template< typename B, typename S >
    struct observer_type_factory : observer_type_factory< B, decltype( &S::connect ) >
    {};

    template< typename B, typename R, typename S, typename ...Ao >
    struct observer_type_factory< B, R ( S:: * )( observer< Ao... > * ) >
    {
        using type = owner_observer< B, S, Ao... >;
    };

    template< typename B, typename R, typename S, typename ...Ao >
    struct observer_type_factory< B, R ( S:: * )( observer< Ao... > * ) const >
    {
        using type = owner_observer< B, S, Ao... >;
    };

    template< typename O, typename F, typename ...Ao >
    class member_function_observer
    {
        O * const m_instance;
        F         m_function;

    protected:
        member_function_observer( O * instance, F function )
                : m_instance( instance )
                , m_function( function )
        {}

        void invoke( Ao&&... args, ... )
        {
            ( m_instance->*m_function )( std::forward< Ao >( args )... );
        }
    };

    template< typename F >
    class function_observer
    {
        F m_function;

    protected:
        function_observer( const F &function )
                : m_function( function )
        {}

        template< typename ...As >
        void invoke( As&&... args )
        {
            detail::invoke_helper< F >::invoke( m_function, std::forward< As >( args )... );
        }
    };

    observer_owner( const observer_owner & ) = delete;
    observer_owner & operator=( const observer_owner & ) = delete;

    std::set< abstract_owner_observer * > m_observers;

    void remove_observer( abstract_owner_observer * const o ) noexcept
    {
        if( m_observers.erase( o ) )
        {
            delete o;
        }
    }

    void add_observer( abstract_owner_observer * const o ) noexcept
    {
        m_observers.insert( o );
    }

public:
    /**
     * \brief A handle to a subject <--> observer connection.
     *
     * \note Reuse of this handle may lead to undefined behavior.
     *
     * \see observer_owner::connect observer_owner::disconnect
     */
    class connection
    {
        friend observer_owner;
        abstract_owner_observer * m_h = nullptr;

        connection( abstract_owner_observer * h )
                : m_h( h )
        {}

    public:
        connection() noexcept = default;
    };

    observer_owner() = default;

    ~observer_owner() noexcept
    {
        for( auto o : m_observers )
        {
            o->remove_from_subject();
            delete o;
        }
    }

    /**
     * \brief Connects a member function of an object to a subject.
     *
     * \param s        The subject from which the object's member function will receive notifications.
     * \param instance The instance of the object.
     * \param function The member function pointer that is called when the subject notifies
     *
     * \return Returns an observer_owner::connection handle.
     *
     * The number of parameter that \em function accepts can be less than the number of values that comes with the notification.
     *
     * \note The lifetime of the instance must exceed the observer_owner's lifetime.
     */
    template< typename S, typename R, typename O, typename ...Ao >
    connection connect( S &s, O * instance, R ( O::* const function )( Ao... ) ) noexcept
    {
        using observer_type = typename observer_type_factory< member_function_observer< O, R ( O::* )( Ao... ), Ao... >, S >::type;
        return new observer_type( *this, s, instance, function );
    }

    template< typename S, typename R, typename O, typename ...Ao >
    connection connect( S &s, O * instance, R ( O::* const function )( Ao... ) const ) noexcept
    {
        using observer_type = typename observer_type_factory< member_function_observer< O, R ( O::* )( Ao... ) const, Ao... >, S >::type;
        return new observer_type( *this, s, instance, function );
    }

    /**
     * \brief Connects a callable to a subject.
     *
     * \param s        The subject from which \em function will receive notifications.
     * \param function A callable such as a free function, lambda, std::function or a functor that is called when the subject notifies.
     *
     * \return Returns a connection handle.
     *
     * The number of parameter that \em function accepts can be less than the number of values that comes with the notification.
     *
     * The \em function is copied and stored in the observer_owner.
     * This means that the callables must have a copy constructor.
     *
     * \note When a callable has side effects than the lifetime of these side effects must exceed the observer_owner's lifetime.
     */
    template< typename S, typename F >
    connection connect( S &s, F function ) noexcept
    {
        using observer_type = typename observer_type_factory< function_observer< F >, S >::type;
        return new observer_type( *this, s, function );
    }

    /**
     * \brief Disconnects the observer from its subject.
     *
     * \param c The connection handle.
     *
     * The observer will be deleted in case it is a lambda, std::function or a functor.
     *
     * \see observer_owner::connect
     */
    void disconnect( const connection &c ) noexcept
    {
        // Disconnect only when its our connection.
        auto it = m_observers.find( c.m_h );
        if( it != m_observers.cend() )
        {
            c.m_h->remove_from_subject();
            m_observers.erase( it );
            delete c.m_h;
        }
    }
};

}

