//
// Created by svakhreev on 05.06.17.
//

#ifndef POLYMORPHISMTESTS_UTILITY_HPP
#define POLYMORPHISMTESTS_UTILITY_HPP

#include <type_traits>
#include <functional>
#include <variant>
#include <optional>

namespace staticpoly::utility
{

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template<typename Func, typename... Args>
constexpr bool is_generic()
{
    return (... && std::is_invocable_v<Func, Args>);
};

template<typename Func, typename First, typename... Args>
constexpr bool is_generic_2nd()
{
    return (... && std::is_invocable_v<Func, First, Args>);
};

template < typename Func, typename Ret, typename _, typename A, typename... Rest >
A _sec_arg_hlpr(Ret (Func::*)(_, A, Rest...));

template < typename Func, typename Ret, typename _, typename A, typename... Rest >
A _sec_arg_hlpr(Ret (Func::*)(_, A, Rest...) const);

template < typename Func >
using second_argument = decltype(_sec_arg_hlpr(&Func::operator()));

template < typename... Types, typename Func, std::size_t... I >
constexpr auto tuple_transform_impl(std::tuple<Types...> t, Func&& func, std::index_sequence<I...>)
{
    return std::make_tuple(
        func(std::forward<std::remove_reference_t<decltype(std::get<I>(t))>>(
            std::get<I>(t)))...);
}

template < typename... Types, typename Func >
constexpr auto tuple_transform(std::tuple<Types...> t, Func&& f)
{
    return tuple_transform_impl(t, std::forward<Func>(f), std::make_index_sequence<sizeof...(Types)>{});
}

template < typename AccType, typename... Callable >
constexpr auto tup_funcs(AccType& initial_acc, Callable&&... funcs)
{
    return tuple_transform(std::tuple<Callable...>{ std::forward<Callable>(funcs)... },
        [&initial_acc](auto&& func) {
            return [&initial_acc, &func] (second_argument<std::remove_reference_t<decltype(func)>> arg) {
            return func(initial_acc, std::forward<decltype(arg)>(arg)); };
        });
}

template < typename AccType, typename... Callable >
constexpr auto tup_funcs_generic(AccType& initial_acc, Callable&&... funcs)
{
    return tuple_transform(std::tuple<Callable...>{ std::forward<Callable>(funcs)... },
        [&initial_acc](auto&& func) {
            return [&initial_acc, &func] (auto arg) {
                return func(initial_acc, std::forward<decltype(arg)>(arg)); };
        });
}

template < typename Func, typename Ret, typename A, typename... Rest >
A _first_arg_hlpr(Ret (Func::*)(A, Rest...));

template <typename... Args, size_t... Indexes, typename... ExtraCallable>
constexpr auto make_overloaded_from_tup(std::tuple<Args...> t,
                                        std::index_sequence<Indexes...>,
                                        ExtraCallable&&... extra)
{
    return overloaded { std::forward<decltype(std::get<Indexes>(t))>(std::get<Indexes>(t))...,
                        std::forward<std::remove_reference_t<ExtraCallable>>(extra)... };
}

template < typename Func, typename Ret, typename A, typename... Rest >
A _first_arg_hlpr(Ret (Func::*)(A, Rest...) const);

template < typename Func >
using first_argument = decltype(_first_arg_hlpr(&Func::operator()));

template < typename... Callable >
constexpr auto tup_funcs_opt(Callable&&... funcs)
{
    return tuple_transform(std::tuple<Callable...>{ std::forward<Callable...>(funcs)... }, [](auto&& func) {
        return [&func] (first_argument<std::remove_reference_t<decltype(func)>> arg) {
        return std::make_optional(func(std::forward<std::remove_reference_t<decltype(arg)>>(arg))); };
    });
}

}

#endif //POLYMORPHISMTESTS_UTILITY_HPP
