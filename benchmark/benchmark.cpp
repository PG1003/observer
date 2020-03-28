#include <observer.h>
#include <functional>
#include <chrono>
#include <cstdio>

using namespace std;
using namespace std::chrono;

volatile int count_value = 0;
int iterations  = 1000000;
int increment   = 1;


void increase_count( int value )
{
    count_value += value;
}

struct increase_functor
{
    int m_value = 0;

    increase_functor( int value )
            : m_value( value )
    {}

    void operator()( int )
    {
        count_value += m_value;
    }
};

struct increase
{
    int m_value = 0;

    increase( int value )
            : m_value( value )
    {}

    void increase_count( int )
    {
        count_value += m_value;
    }
};

int main( int /* argc */, char * /* argv */[] )
{
    pg::connection_owner owner;

    std::function< void( int ) > increase_count_std_function = [&]( int value ){ count_value += value; };
    auto increase_count_lambda_function                      = [&]( int value ){ count_value += value; };
    auto increase_count_functor                              = increase_functor( increment );
    auto increase_member_function                            = increase( increment );

    // Get the baseline
    const auto baseline_start_free_function = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        increase_count( increment );
    }
    const auto baseline_stop_free_function = std::chrono::steady_clock::now();

    const auto baseline_start_std_function = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        increase_count_std_function( increment );
    }
    const auto baseline_stop_std_function = std::chrono::steady_clock::now();

    const auto baseline_start_lambda = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        increase_count_lambda_function( increment );
    }
    const auto baseline_stop_lambda = std::chrono::steady_clock::now();

    const auto baseline_start_functor = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        increase_count_functor( increment );
    }
    const auto baseline_stop_functor = std::chrono::steady_clock::now();

    const auto baseline_start_member_function = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        increase_member_function.increase_count( increment );
    }
    const auto baseline_stop_member_function = std::chrono::steady_clock::now();

    // With observers
    pg::subject< int > subject_free_function;
    owner.connect( subject_free_function, increase_count );
    const auto start_free_function = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        subject_free_function.notify( increment );
    }
    const auto stop_free_function = std::chrono::steady_clock::now();

    pg::subject< int > subject_std_function;
    owner.connect( subject_std_function, increase_count_std_function );
    const auto start_std_function = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        subject_std_function.notify( increment );
    }
    const auto stop_std_function = std::chrono::steady_clock::now();

    pg::subject< int > subject_lambda;
    owner.connect( subject_lambda, [&]( int value ){ count_value += value; } );
    const auto start_lambda = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        subject_lambda.notify( increment );
    }
    const auto stop_lambda = std::chrono::steady_clock::now();

    pg::subject< int > subject_functor;
    owner.connect( subject_functor, increase_count_functor );
    const auto start_functor = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        subject_functor.notify( increment );
    }
    const auto stop_functor = std::chrono::steady_clock::now();

    pg::subject< int > subject_member_function;
    owner.connect( subject_member_function, &increase_member_function, &increase::increase_count );
    const auto start_member_function = std::chrono::steady_clock::now();
    for( int i = 0 ; i < iterations ; ++i )
    {
        subject_member_function.notify( increment );
    }
    const auto stop_member_function = std::chrono::steady_clock::now();


    struct result
    {
        using time_point = std::chrono::steady_clock::time_point;
        using time       = std::chrono::duration< float, std::micro >;

        const float m_base_time;
        const float m_time;
        const float m_difference;

        result( const time_point & base_start,
                const time_point & base_stop,
                const time_point & start,
                const time_point & stop )
            : m_base_time( time( base_stop - base_start ).count() )
            , m_time( time( stop - start ).count() )
            , m_difference( time( stop - start ).count() / time( base_stop - base_start ).count() )
        {}
    };

    auto free_function_result   = result( baseline_start_free_function,   baseline_stop_free_function,   start_free_function,   stop_free_function );
    auto std_function_result    = result( baseline_start_std_function,    baseline_stop_std_function,    start_std_function,    stop_std_function );
    auto lambda_result          = result( baseline_start_lambda,          baseline_stop_lambda,          start_lambda,          stop_lambda );
    auto functor_result         = result( baseline_start_functor,         baseline_stop_functor,         start_functor,         stop_functor );
    auto member_function_result = result( baseline_start_member_function, baseline_stop_member_function, start_member_function, stop_member_function );

    printf(
        "|---------------------------------------------------------\n"
        "|                 |  baseline  |  observer  | difference |\n"
        "|---------------------------------------------------------\n"
        "| free function   | %10.2f | %10.2f | %9.2fx |\n"
        "| std::function   | %10.2f | %10.2f | %9.2fx |\n"
        "| lambda          | %10.2f | %10.2f | %9.2fx |\n"
        "| functor         | %10.2f | %10.2f | %9.2fx |\n"
        "| member function | %10.2f | %10.2f | %9.2fx |\n"
        "----------------------------------------------------------\n",
        free_function_result.m_base_time,   free_function_result.m_time,  free_function_result.m_difference,
        std_function_result.m_base_time,    std_function_result.m_time,   std_function_result.m_difference,
        lambda_result.m_base_time,          lambda_result.m_time,         lambda_result.m_difference,
        functor_result.m_base_time,         functor_result.m_time,        functor_result.m_difference,
        member_function_result.m_base_time, member_function_result.m_time, member_function_result.m_difference );

    return 0;
}
