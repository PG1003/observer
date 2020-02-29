// MIT License
//
// Copyright (c) 2019 PG1003
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

namespace detail
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

template< typename F, typename ...A >
inline void invoke( const F function, A... args )
{
    invoke_helper< F >::invoke( function, std::forward< A >( args )... );
}

}

template< typename ...A >
class subject;

template< typename S >
class subject_blocker;

template< typename ...A >
class observer
{
public:
    virtual ~observer() noexcept = default;

    virtual void disconnect( subject< A... > * ) noexcept = 0;
    virtual void notify( A... args ) = 0;
};

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

    void connect( observer_type * const o ) noexcept
    {
        m_observers.push_back( o );
    }

    void disconnect( const observer_type * const o ) noexcept
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

    void notify( A... args ) const
    {
        for( auto o : m_observers )
        {
            o->notify( args... );
        }
    }
};

class observer_owner
{
    class abstract_owner_handle;

public:
    class connection
    {
        // Do not let others inherit from this class
        friend class abstract_owner_handle;

    private:
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
                detail::invoke( m_function, std::forward< As >( args )... );
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

    template< typename ...As1, typename ...As2 >
    connection * connect( subject< As1... > &s1, subject< As2... > &s2 ) noexcept
    {
        return connect( s1, [&]( As2 &&... args ){ s2.notify( std::forward< As2 >( args )... ); } );
    }

    void disconnect( connection * const c ) noexcept
    {
        auto o = static_cast< abstract_owner_handle * >( c );
        o->remove_from_subject();
        remove_observer( o );
    }
};

// The use of observer_handle has been deprecated, use observer_owner::connection instead!
using observer_handle = typename observer_owner::connection;

template< typename S >
class subject_blocker
{
    subject_blocker( const subject_blocker< S > & ) = delete;
    subject_blocker & operator=( const subject_blocker< S > & ) = delete;

    S                                          &m_subject;
    std::vector< typename S::observer_type * > m_observers;

public:
    subject_blocker( subject_blocker< S > && ) = default;

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
