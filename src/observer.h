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
    return invoke_helper< F >::invoke( function, std::forward< A >( args )... );
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
    virtual void disconnect( const subject< A... > * s ) noexcept = 0;

    /**
     *  \brief The notification function that is called when the subject notifies
     *
     *  \param args The values of the notification. These are defined by this class' template parameters.
     */
    virtual void notify( A... args ) = 0;
};

/**
 * \brief Class that calls the notification function of its observers.
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

    friend class subject_blocker< subject< A... > >;

    std::vector< observer_type * > m_observers;

public:
    subject() noexcept = default;

    ~subject() noexcept
    {
        for( auto it = m_observers.rbegin() ; it != m_observers.crend() ; ++it )
        {
            ( *it )->disconnect( this );
        }
    }

    /**
     * \brief Connects a observer to the subject.
     *
     * \param o The observer to connects to the subject.
     *
     * The subject doesn't take ownership of the the observer.
     * The destructor calls the disconnect of its observers in the reversed order the in which observers were connected.
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
 * \brief Manages the subject <--> observer connection lifetime from the observer side.
 *
 * Connections that are made and thus owned by observer_owner are removed at destruction of observer_owner.
 */
class observer_owner
{
public:
    class connection;

private:
    class abstract_owner_observer
    {
    public:
        connection * m_connection = nullptr;

        virtual ~abstract_owner_observer() noexcept;
        virtual void remove_from_subject() noexcept = 0;
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

public:
    /**
     * \brief A handle to a subject <--> observer connection.
     *
     * Connection handles are invalidated when they are used to disconnect or when the observer_owner is destroyed.
     *
     * There is only one instance of a connection handle created for each connection, they cannot be copied.
     * However, a std::shared_ptr can be used when a connection handle needs to be shared between multiple objects.
     *
     * \code{.cpp}
     * auto shared_connection = std::make_shared< observer_owner::connection >( my_observer_owner.connect( my_subject, my_function ) );
     * \endcode
     *
     * \see observer_owner::connect observer_owner::disconnect
     */
    class connection
    {
        mutable abstract_owner_observer * m_observer = nullptr;

        // Only let observer_owner create valid connection handles.
        friend class observer_owner;
        connection( abstract_owner_observer * o ) noexcept
                : m_observer( o )
        {
            m_observer->m_connection = this;
        }

    public:
    	connection() noexcept = default;

        connection & operator=( connection && c ) noexcept
        {
            if( c.m_observer )
            {
                m_observer               = c.m_observer;
                m_observer->m_connection = this;
                c.m_observer             = nullptr;
            }

            return *this;
        }

        connection( connection && c ) noexcept
        {
            if( c.m_observer )
            {
                m_observer               = c.m_observer;
                m_observer->m_connection = this;
                c.m_observer             = nullptr;
            }
        }

        ~connection() noexcept
        {
            if( m_observer )
            {
                m_observer->m_connection = nullptr;
            }
        }
    };

    observer_owner() noexcept = default;

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
    connection connect( subject< As... > &s, I * instance, R ( I::* const function )( Ao... ) ) noexcept
    {
        class owner_observer final : public observer< As ... >, public abstract_owner_observer
        {
            observer_owner   &m_owner;
            subject< As... > &m_subject;
            I * const        m_instance;
            R( I::* const    m_function )( Ao... );

            void invoke( Ao... args, ... )
            {
                ( m_instance->*m_function )( std::forward< Ao >( args )... );
            }

            virtual void notify( As... args ) override
            {
                invoke( std::forward< As >( args )... );
            }

            virtual void disconnect( const subject< As... > * ) noexcept override
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
    connection connect( subject< As... > &s, const F function ) noexcept
    {
        class owner_observer final : public observer< As... >, public abstract_owner_observer
        {
            observer_owner   &m_owner;
            subject< As... > &m_subject;
            const F          m_function;

            virtual void notify( As... args ) override
            {
                pg::invoke( m_function, std::forward< As >( args )... );
            }

            virtual void disconnect( const subject< As... > * ) noexcept override
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
     * \param s2 The subject that notifies its observers when the subject to which it is connected notifies
     *
     * \return Returns an observer_owner::connection handle.
     *
     * The number of paramemters that \em s2 accepts in its notify function can be less than number of values
     * that comes with the notificatoin from \em s1.
     *
     * \note The lifetime of the notified subject and its side effects must exceed the observer_owner's lifetime.
     */
    template< typename ...As1, typename ...As2 >
    connection connect( subject< As1... > &s1, subject< As2... > &s2 ) noexcept
    {
        return connect( s1, [&]( As2 &&... args ){ s2.notify( std::forward< As2 >( args )... ); } );
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
        if( c.m_observer )
        {
        	// Disconnect only when its our connection.
            auto it = m_observers.find( c.m_observer );
            if( it != m_observers.cend() )
            {
                c.m_observer->remove_from_subject();
                m_observers.erase( it );
                delete c.m_observer;
            }
        }
    }
};

observer_owner::abstract_owner_observer::~abstract_owner_observer()
{
    if( m_connection )
    {
        // Invalidate connection handle
        m_connection->m_observer = nullptr;
    }
}

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
