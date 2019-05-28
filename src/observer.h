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
#include <memory>
#include <utility>


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

class observer_owner;

template< typename ...A >
class subject;

template< typename S >
class subject_blocker;

class observer_handle
{
    template< typename ...A >
    friend class subject;
    friend class observer_owner;

    observer_owner& m_owner;

    virtual void remove_from_subject() noexcept = 0;

    void remove_from_owner() noexcept;

public:
    explicit observer_handle( observer_owner& owner ) noexcept
            : m_owner( owner )
    {}

    virtual ~observer_handle() noexcept = default;
};

namespace detail
{

template< typename ...A >
class abstract_observer : public observer_handle
{
    subject< A... > &m_subject;

    virtual void remove_from_subject() noexcept final
    {
        m_subject.remove_observer( this );
    }

public:
    explicit abstract_observer( observer_owner& owner, subject< A... > &s ) noexcept
            : observer_handle( owner )
            , m_subject( s )
    {}

    virtual void notify( A... args ) = 0;
};

}

template< typename ...A >
class subject
{
    using observer_type = detail::abstract_observer< A... >;

    friend class detail::abstract_observer< A... >;
    friend class subject_blocker< subject< A... > >;
    friend class observer_owner;

    std::vector< observer_type * > m_observers;

    void remove_observer( observer_handle * const o ) noexcept
    {
        // Iterate reversed over the m_observers since we expect that observers that
        // are frequently connected and disconnected resides at the end of the vector.
        auto it_find = std::find_if( m_observers.crbegin(), m_observers.crend(), [o]( const auto &o1 )
        {
            return static_cast< observer_handle * >( o1 ) == o;
        } );
        if( it_find != m_observers.crend() )
        {
            m_observers.erase( ( ++it_find ).base() );
        }
    }

    void add_observer( detail::abstract_observer< A... > * const o ) noexcept
    {
        m_observers.push_back( o );
    }

public:
    ~subject() noexcept
    {
        for( auto& o : m_observers )
        {
            o->remove_from_owner();
        }
    }

    void notify( A... args ) const
    {
        for( auto& o : m_observers )
        {
            o->notify( args... );
        }
    }
};

class observer_owner
{
    friend class observer_handle;

    struct observer_handle_ptr_comp
    {
        using is_transparent = std::true_type;
        using handle_uptr    = std::unique_ptr< observer_handle >;

        bool operator()( const handle_uptr& lhs, const handle_uptr& rhs ) const noexcept { return lhs < rhs; }
        bool operator()( const handle_uptr& lhs, const observer_handle * const rhs ) const noexcept { return lhs.get() < rhs; }
        bool operator()( const observer_handle * const lhs, const handle_uptr& rhs ) const noexcept { return lhs < rhs.get(); }
    };

    std::set< std::unique_ptr< observer_handle >, observer_handle_ptr_comp > m_observers;

    void remove_observer( observer_handle * const o ) noexcept
    {
        auto it_find = m_observers.find( o );
        m_observers.erase( it_find );
    }

    template< typename ...A >
    observer_handle * connect_( subject< A... > &s, detail::abstract_observer< A... > * const o ) noexcept
    {
        s.add_observer( o );
        m_observers.insert( std::unique_ptr< observer_handle >( o ) );

        return o;
    }

public:
    ~observer_owner() noexcept
    {
        for( auto& o : m_observers )
        {
            o->remove_from_subject();
        }
    }

    template< typename I, typename R, typename ...As, typename ...Ao >
    observer_handle * connect( subject< As... > &s, I * instance, R ( I::* const function )( Ao... ) ) noexcept
    {
        class observer final : public detail::abstract_observer< As ... >
        {
            I * const     m_instance;
            R( I::* const m_function )( Ao... );

            void invoke( Ao... args, ... )
            {
                ( m_instance->*m_function )( std::forward< Ao >( args )... );
            }

        public:
            observer( observer_owner &owner, subject< As... > &s, I * const instance, R ( I::* const f )( Ao... ) ) noexcept
                    : detail::abstract_observer< As... >( owner, s )
                    , m_instance( instance )
                    , m_function( f )
            {}

            virtual void notify( As... args ) override
            {
                invoke( std::forward< As >( args )... );
            }
        };

        return connect_( s, new observer( *this, s, instance, function ) );
    }

    template< typename F, typename ...As >
    observer_handle * connect( subject< As... > &s, const F function ) noexcept
    {
        class observer final : public detail::abstract_observer< As... >
        {
            const F m_function;

        public:
            observer( observer_owner &owner, subject< As... > &s, const F f ) noexcept
                    : detail::abstract_observer< As... >( owner, s )
                    , m_function( f )
            {}

            virtual void notify( As... args ) override
            {
                detail::invoke( m_function, std::forward< As >( args )... );
            }
        };

        return connect_( s, new observer( *this, s, function ) );
    }

    template< typename ...As1, typename ...As2 >
    observer_handle * connect( subject< As1... > &s1, subject< As2... > &s2 ) noexcept
    {
        return connect( s1, [&]( As2 &&... args ){ s2.notify( std::forward< As2 >( args )... ); } );
    }

    void disconnect( observer_handle * const o ) noexcept
    {
        o->remove_from_subject();
        remove_observer( o );
    }
};


void observer_handle::remove_from_owner() noexcept
{
    m_owner.remove_observer( this );
}


template< typename S >
class subject_blocker
{
    S                                          &m_subject;
    std::vector< typename S::observer_type * > m_observers;

public:
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
