#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <algorithm>
#include <numeric>
#include <atomic>
#include <any>
#include <type_traits>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <experimental/iterator>
#include <range/v3/all.hpp>

using namespace std;

template <typename T>
concept bool SwitchableConcept = requires(T a)
{
    a.SwitchOn();
    a.SwitchOff();
};

template <typename... Ts>
concept bool SwitchablesConcept = requires(Ts... a)
{
    ((a.SwitchOn()), ...);
    ((a.SwitchOff()), ...);
};

static int global_id = 0;

struct HomeObject
{
    int id = global_id++;
    string name = "";
    bool isOn = false;
};

struct Iron : public HomeObject
{
    int temperature = 500;

    void SwitchOn()
    {
//        cout << "Iron is on" << endl;
        isOn = true;
    }

    void SwitchOff()
    {
//        cout << "Iron is off" << endl;
        isOn = false;
    }
};

struct Plate : public HomeObject
{
    int plates_count = 4;

    void SwitchOn()
    {
//        cout << "Plate is on" << endl;
        isOn = true;
    }

    void SwitchOff()
    {
//        cout << "Plate is off" << endl;
        isOn = false;
    }
};

struct Spoon : public HomeObject
{
    void Take()
    {
        cout << "Take spoon" << endl;
    }
};

template <typename... Args> requires SwitchablesConcept<Args...>
using SwitchableVariant = variant<Args...>;

using Switchable = SwitchableVariant<Iron, Plate>;


namespace staticpoly
{

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


template < typename InputIter,
           typename InputSentinelIter,
           typename... Callable >
constexpr void apply(InputIter start,
                     InputSentinelIter stop,
                     Callable... funcs)
{
    if (start == stop) return;
    auto it_ = start;
    while (it_ != stop)
        visit(overloaded { funcs..., [](...){} }, *it_++);
};


template < typename InputIter,
           typename InputSentinelIter,
           typename OutputIter,
           typename... Callable >
constexpr void filter(InputIter start,
                      InputSentinelIter stop,
                      OutputIter out,
                      Callable... funcs)
{
    if (start == stop) return;
    auto it_ = start;
    while (it_ != stop)
    {
        if (visit(overloaded{ funcs..., [](...) { return false; } },
                  *it_))
            *out++ = *it_++;
        else
            it_++;
    }
};



template<typename F, typename Ret, typename _, typename A, typename... Rest>
A
second_helper(Ret (F::*)(_, A, Rest...));

template<typename F, typename Ret, typename _, typename A, typename... Rest>
A
second_helper(Ret (F::*)(_, A, Rest...) const);

void second_helper(...);

template<typename F>
struct second_argument_impl
{
    using type = decltype( second_helper(&F::operator()) );
};

template < typename F >
using second_argument = typename second_argument_impl<F>::type;

template < typename Func, typename Check = std::conditional_t < is_same_v <second_argument<Func>, void>,
                                                                std::true_type,
                                                                std::false_type >
         >
struct is_generic_func;

template < typename Func >
struct is_generic_func<Func, true_type> : true_type {};

template < typename Func >
struct is_generic_func<Func, false_type> : false_type {};

template <typename Func>
constexpr bool is_generic_func_v = is_generic_func<Func>::value;


template < typename... Types, typename Func, std::size_t... I >
constexpr auto tuple_transform_impl(std::tuple<Types...> t, Func&& func, std::index_sequence<I...>)
{
    return std::make_tuple(func(std::get<I>(t))...);
}

template < typename... Types, typename Func >
constexpr auto tuple_transform(std::tuple<Types...> t, Func&& f)
{
    return tuple_transform_impl(t, std::forward<Func>(f), std::make_index_sequence<sizeof...(Types)>{});
}

template < typename AccType, typename... Callable >
constexpr auto tup_funcs(AccType& initial_acc, Callable... funcs)
{
    return tuple_transform(tuple<Callable...>{ funcs... }, [&initial_acc](auto func) {
        return [&initial_acc, &func] (second_argument<decltype(func)> arg) { return func(initial_acc, arg); };
    });
}

template <typename... Args, size_t... Indexes, typename... ExtraCallable>
constexpr auto make_overloaded_from_tup(tuple<Args...> t, index_sequence<Indexes...>, ExtraCallable... extra)
{
    return overloaded { get<Indexes>(t)..., extra...};
}

template < typename InputIter,
           typename InputSentinelIter,
           typename AccType,
           typename... Callable,
           typename = enable_if_t<!(... || is_generic_func_v<Callable>), void> >
constexpr auto reduce(InputIter start,
                      InputSentinelIter stop,
                      AccType initial_acc,
                      Callable... funcs)
{
    static_assert(!(... || is_generic_func_v<Callable>), "Generic lambdas is not supported");

    if (start == stop) return initial_acc;
    auto it_ = start;
    while (it_ != stop)
    {
        initial_acc = visit(make_overloaded_from_tup(tup_funcs(initial_acc, funcs...),
                                                     make_index_sequence<sizeof...(Callable)>{},
                                                     [&initial_acc](...) { return initial_acc; } ),
                            *it_++);
    }
    return initial_acc;
};

template < typename InputIter,
           typename InputSentinelIter,
           typename AccType,
           typename Callable,
           typename = enable_if_t<is_generic_func_v<Callable>, void> >
constexpr auto reduce(InputIter start,
                      InputSentinelIter stop,
                      AccType initial_acc,
                      Callable func)
{
    if (start == stop) return initial_acc;
    auto it_ = start;
    while (it_ != stop)
    {
        initial_acc = visit(overloaded {
            [&initial_acc, &func] (auto arg) { return func(initial_acc, arg); } });
    }
    return initial_acc;
};




template<typename F, typename Ret, typename A, typename... Rest>
A
first_helper(Ret (F::*)(A, Rest...));

template<typename F, typename Ret, typename A, typename... Rest>
A
first_helper(Ret (F::*)(A, Rest...) const);

template<typename F>
struct first_argument_impl
{
    typedef decltype( first_helper(&F::operator()) ) type;
};

template < typename F >
using first_argument = typename first_argument_impl<F>::type;

template < typename... Callable >
constexpr auto tup_funcs_opt(Callable... funcs)
{
    return tuple_transform(tuple<Callable...>{ funcs... }, [](auto func) {
        return [&func] (first_argument<decltype(func)> arg) { return make_optional(func(arg)); };
    });
}

template < typename InputIter,
           typename InputSentinelIter,
           typename OutputIter,
           typename... Callable >
constexpr void map(InputIter start,
                   InputSentinelIter stop,
                   OutputIter out,
                   Callable... funcs)
{
    using ValueType = typename OutputIter::container_type::value_type;
    using ValueOptionalType = optional<ValueType>;

    if (start == stop) return;
    auto it_ = start;
    while (it_ != stop)
    {

        auto maybe_value = visit(make_overloaded_from_tup(tup_funcs_opt(funcs...),
                                                          make_index_sequence<sizeof...(Callable)>{},
                                                          [](...){ return ValueOptionalType{}; }),
                                 *it_++);

        if (maybe_value)
            *out++ = maybe_value.value();
        else
            out++;
    }
};

}






int main()
{
    std::srand(unsigned(std::time(0)));
    vector<uint64_t> times = {};

    vector<Switchable> home_objects;
    home_objects.reserve(1000);

    for (int i = 0; i < 1000; ++i)
    {
        if (std::rand() % 2 == 1)
            home_objects.emplace_back(Iron{});
        else
            home_objects.emplace_back(Plate{});
    }

    vector<Switchable> filtered;
    staticpoly::filter(home_objects.begin(), home_objects.end(),
                       back_inserter(filtered),
                       [] (Iron item) { return item.id > 50 and item.temperature == 500; },
                       [] (Plate item) { return item.id > 650 and item.plates_count == 4; });
    cout << filtered.size() << endl;


    staticpoly::apply(home_objects.begin(),
                      home_objects.end(),
                      [](auto& item) { item.SwitchOn(); });

    staticpoly::apply(home_objects.begin(),
                      home_objects.end(),
                      [](auto item) { assert(item.isOn); });


    auto result = staticpoly::reduce(home_objects.begin(), home_objects.end(),
                                     0,
                                     [] (int acc, auto item) { return item.isOn ? ++acc : acc; });
    cout << result << endl;



    vector<Iron> ids;
    staticpoly::map(home_objects.begin(), home_objects.end(),
                    back_inserter(ids),
                    [] (Iron item) { return item; });

    for (auto i: ids)
    {
        std::cout << i.id << std::endl;
    }


//    for (auto i = 0; i < 1000; ++i)
//    {
//        auto start = chrono::system_clock::now();
//
//
//        auto stop = chrono::system_clock::now();
//
//        times.emplace_back(chrono::duration_cast<chrono::microseconds>(stop - start).count());
//    }
//
//    cout << accumulate(times.begin(), times.end(), 0) / 1000 << endl;

    return 0;
}