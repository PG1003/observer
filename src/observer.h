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

namespace
{

// https://stackoverflow.com/a/27867127
template< typename T >
struct invoke_helper : invoke_helper< decltype( &T::operator() ) > {};

template< typename R, typename ...A >
struct invoke_helper< R ( * )( A... ) >
{
    template< typename F >
    static void invoke( const F function, A... args, ... )
    {
        function( std::forward< A >( args )... );
    }
};

template< typename R, typename C, typename ...A >
struct invoke_helper< R ( C:: * )( A... ) >
{
    template< typename F >
    static void invoke( F function, A... args, ... )
    {
        function( std::forward< A >( args )... );
    }
};

template< typename R, typename C, typename ...A >
struct invoke_helper< R ( C:: * )( A... ) const >
{
    template< typename F >
    static void invoke( const F function, A... args, ... )
    {
        function( std::forward< A >( args )... );
    }
};

}

/**
 * \brief Invokes a callable with the given arguments.
 *
 * \param function A callable such as a free function, lambda, std::function or a functor.
 * \param args     The arguments to forward to \em function.
 *
 * The advantage this implementation over std::invoke is that the number of parameters that is
 * forwarded to \em function is adjusted when \em function accept less parameters than passed to \em invoke.
 */
template< typename F, typename ...A >
inline void invoke( const F function, A... args )
{
    invoke_helper< F >::invoke( function, std::forward< A >( args )... );
}

template< typename ...A >
class subject;

template< typename S >
class subject_blocker;

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
     * \param s The subject from which the observer is disconnected.
     *
     * The subject calls disconnect on all its observers when the subject gets deleted or goes out of scope.
     */
    virtual void disconnect( subject< A... > * s ) noexcept = 0;

    /**
     *  \brief The notification function that is called when the subject notifies
     *
     *  \param args The values of the notification. These are defined by this class' template parameters.
     */
    virtual void notify( A... args ) = 0;
};

/**
 * \brief Class that calls the notification function of it's observers.
 *
 * \tparam A The types of the values that are passed to the observers notification functions.
 *
 * \see observer
 */
template< typename ...A >
class subject
{
    using observer_type = observer< A... >;

    subject( const subject< A... > & ) = delete;
    subject & operator=( const subject< A... > & ) = delete;
    subject( subject< A... > && ) = delete;

    friend class subject_blocker< subject< A... > >;

    std::vector< observer_type * > m_observers;

public:
    subject() = default;

    ~subject() noexcept
    {
        for( auto o : m_observers )
        {
            o->disconnect( this );
        }
    }

    /**
     * \brief Connects a observer to the subject.
     *
     * \param o The observer to connects to the subject.
     *
     * The subject doesn't take ownership of the the observer.
     * The disconnect function of the connected observers is called at destruction of the subject.
     *
     * \note This function doesn't check if the observer was already connected.
     *       For example when an observer is connected 3 times then it will be notified 3 times.
     */
    void connect( observer< A... > * const o ) noexcept
    {
        m_observers.push_back( o );
    }

    /**
     *  \brief Disconnects a observer from the subject.
     *
     *  \param o The observer to disconnect from the subject.
     *
     *  \note It is possible to connect the same observer a multiple times.
     *        When this is the case, the observer must to be disconnected the same number of times it was connected.
     */
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

    /**
     * \brief Notifies the observers observers connected to this subject.
     *
     * \param args The values passed to the observer's notification function.
     *
     * The observers are notified in the order they are connected.
     */
    void notify( A... args ) const
    {
        for( auto o : m_observers )
        {
            o->notify( args... );
        }
    }
};

/**
 * \brief Manages the subject - observer connection lifetime from the observer side.
 *
 * Connections that are made and thus owned by observer_owner are removed at destruction of observer_owner.
 * The observer is automatically disconnected from it's subject when the connection is removed.
 */
class observer_owner
{
    class abstract_owner_handle;

public:
    /**
     * \brief A handle to a subject - observer connection.
     *
     * \warning You shall not inherit from this class.
     *
     * \see observer_owner::connect observer_owner::disconnect
     */
    class connection
    {
        // Do not let others inherit from this class
        friend class abstract_owner_handle;

        explicit connection() = default;
    };

private:
    observer_owner( const observer_owner & ) = delete;
    observer_owner & operator=( const observer_owner & ) = delete;
    observer_owner( observer_owner && ) = delete;

    class abstract_owner_handle : public connection
    {
    public:
        virtual ~abstract_owner_handle() = default;
        virtual void remove_from_subject() noexcept = 0;
    };

    std::set< abstract_owner_handle * > m_observers;

    void remove_observer( abstract_owner_handle * const o ) noexcept
    {
        if( m_observers.erase( o ) )
        {
            delete o;
        }
    }

public:
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
    template< typename I, typename R, typename ...As, typename ...Ao >
    connection * connect( subject< As... > &s, I * instance, R ( I::* const function )( Ao... ) ) noexcept
    {
        class owner_observer final : public observer< As ... >, public abstract_owner_handle
        {
            observer_owner   &m_owner;
            subject< As... > &m_subject;
            I * const        m_instance;
            R( I::* const    m_function )( Ao... );

            void invoke( Ao... args, ... )
            {
                ( m_instance->*m_function )( std::forward< Ao >( args )... );
            }

            virtual void notify( As... args ) override final
            {
                invoke( std::forward< As >( args )... );
            }

            virtual void disconnect( subject< As... > * ) noexcept override
            {
                m_owner.remove_observer( this );
            }

            virtual void remove_from_subject() noexcept override
            {
                m_subject.disconnect( this );
            }

        public:
            owner_observer( observer_owner &owner, subject< As... > &s, I * const instance, R ( I::* const f )( Ao... ) ) noexcept
                    : m_owner( owner )
                    , m_subject( s )
                    , m_instance( instance )
                    , m_function( f )
            {
                m_owner.m_observers.insert( this );
                m_subject.connect( this );
            }
        };

        return new owner_observer( *this, s, instance, function );
    }

    /**
     * \brief Connects a callable to a subject.
     *
     * \param s        The subject from which \em function will receive notifications.
     * \param function A callable such as a free function, lambda, std::function or a functor that is called when the subject notifies.
     *
     * \return Returns an observer_owner::connection handle.
     *
     * The number of parameter that \em function accepts can be less than the number of values that comes with the notification.
     *
     * The \em function is copied and stored in the observer_owner.
     * This means that the callables must have a copy constructor.
     *
     * \note When a callable has side effects than the lifetime of these side effects must exceed the observer_owner's lifetime.
     */
    template< typename F, typename ...As >
    connection * connect( subject< As... > &s, const F function ) noexcept
    {
        class owner_observer final : public observer< As... >, public abstract_owner_handle
        {
            observer_owner   &m_owner;
            subject< As... > &m_subject;
            const F          m_function;

            virtual void notify( As... args ) override final
            {
                pg::invoke( m_function, std::forward< As >( args )... );
            }

            virtual void disconnect( subject< As... > * ) noexcept override
            {
                m_owner.remove_observer( this );
            }

            virtual void remove_from_subject() noexcept override
            {
                m_subject.disconnect( this );
            }

        public:
            owner_observer( observer_owner &owner, subject< As... > &s, const F f ) noexcept
                    : m_owner( owner )
                    , m_subject( s )
                    , m_function( f )
            {
                m_owner.m_observers.insert( this );
                m_subject.connect( this );
            }
        };

        return new owner_observer( *this, s, function );
    }

    /**
     * \brief Connects a subject to another subject.
     *
     * \param s1 The subject from which the other subject will be notified.
     * \param s2 The subject that notifies it's observers when the subject to which it is connected notifies
     *
     * \return Returns an observer_owner::connection handle.
     *
     * The number of paramemters that \em s2 accepts in it's notify function can be less than number of values
     * that comes with the notificatoin from \em s1.
     *
     * \note The lifetime of the notified subject and it's side effects must exceed the observer_owner's lifetime.
     */
    template< typename ...As1, typename ...As2 >
    connection * connect( subject< As1... > &s1, subject< As2... > &s2 ) noexcept
    {
        return connect( s1, [&]( As2 &&... args ){ s2.notify( std::forward< As2 >( args )... ); } );
    }

    /**
     * \brief Disconnects the observer from it's subject.
     *
     * \param c The connection handle.
     *
     * The observer will be deleted in case it is a lambda, std::function or a functor.
     *
     * \see observer_owner::connect
     */
    void disconnect( connection * const c ) noexcept
    {
        auto o = static_cast< abstract_owner_handle * >( c );
        o->remove_from_subject();
        remove_observer( o );
    }
};

/**
 * \brief This is an alias for backwards compatibility.
 *
 * Previously an observer_handle was return by the observer_owner::connect functions but is replaced
 * by a more robust observer_owner::connection handle type.
 *
 * \warning The use of observer_handle has been deprecated, use observer_owner::connection instead!
 */
using observer_handle = typename observer_owner::connection;

/**
 * \brief Blocks temporary the notifications of a subject.
 *
 * \tparam S The type of the blocked subject.
 *
 * This class use RAII. The class blocks notifications when it is constructed and unblocked at destruction,
 * for example when a subject_blocker instance leaves it's scope.
 */
template< typename S >
class subject_blocker
{
    subject_blocker( const subject_blocker< S > & ) = delete;
    subject_blocker & operator=( const subject_blocker< S > & ) = delete;

    S                                          &m_subject;
    std::vector< typename S::observer_type * > m_observers;

public:
    subject_blocker( subject_blocker< S > && ) = default;

    /**
     * \param subject A reference to the subject which notifications must be blocked.
     */
    subject_blocker( S &subject ) noexcept
            : m_subject( subject )
    {
        std::swap( m_observers, m_subject.m_observers );
    }

    ~subject_blocker() noexcept
    {
        std::swap( m_observers, m_subject.m_observers );
    }
};

}
