//
// Created by svakhreev on 05.06.17.
//

#ifndef POLYMORPHISMTESTS_FUNCS_HPP
#define POLYMORPHISMTESTS_FUNCS_HPP

#include "utils.hpp"

namespace staticpoly
{


namespace _concepts
{
    template < typename Iterator >
    concept bool IsInputIterator = requires(Iterator it,
                                            typename std::iterator_traits<Iterator>::value_type val)
    {
        it == it;
        it != it;

        *it;
        std::is_same_v<decltype(*it), decltype(val)>;
        it.operator->();

        (void) it++;
        ++it;
        *it++;
    };

    template < typename Iterator >
    concept bool IsOutputIterator = !IsInputIterator<Iterator> &&
        requires(Iterator it)
    {
        *it = std::declval<std::remove_reference_t<decltype(*(std::declval<Iterator>()))>>();
        ++it;
        it++;
        *it++ = std::declval<std::remove_reference_t<decltype(*(std::declval<Iterator>()))>>();
    };

    template < typename T1, typename T2 >
    concept bool IsComparable = requires(T1 t1, T2 t2)
    {
        t1 == t2;
        t1 != t2;
    };

    template < typename Func, typename T >
    concept bool IsCallableWith = requires(Func f, T t)
    {
        f(t);
    };

    template < typename Func, typename Head, typename Tail >
    concept bool IsCallableWithReduce = requires(Func f, Head h, Tail tail)
    {
        f(h, tail);
        std::is_same_v<decltype(f(h, tail)), Head>;
    };

    template < typename Func, typename T, typename Out >
    concept bool IsCallableWithMap = requires(Func f, T value)
    {
        f(value);
        std::is_same_v<decltype(f(value)), Out>;
    };
}

namespace _impl
{
    template <typename F, typename... Types>
    constexpr bool IsCallableWithAnyOf = (... || _concepts::IsCallableWith<F, Types>);

    template <
               typename InputIter,
               typename InputSentinelIter,
               typename... Callable,
               typename... VariantTypes
             >
    requires(_concepts::IsInputIterator<InputIter>, _concepts::IsInputIterator<InputSentinelIter>)
    constexpr void apply_concepts_check(std::variant<VariantTypes...>)
    {
        static_assert((... && std::is_copy_constructible_v<VariantTypes>),
                      "Variant types must be copy-constructible");

        static_assert((... && std::is_copy_assignable_v<VariantTypes>),
                      "Variant types must be copy-assignable");

        static_assert((... && std::is_destructible_v<VariantTypes>),
                      "Variant types must be destructible");

        static_assert(std::is_same_v < typename std::iterator_traits<InputIter>::value_type,
                                       typename std::iterator_traits<InputSentinelIter>::value_type >,
                      "Underlying types of begin and end iterator is not same");

        static_assert(_concepts::IsComparable < InputIter, InputSentinelIter >,
                      "Iterators is not comparable");

        static_assert((... && IsCallableWithAnyOf<Callable, VariantTypes...>),
                      "One of passed functors not callable with all of variant types");
    }
}


template <
           typename InputIter,
           typename InputSentinelIter,
           typename... Callable
         >
constexpr void apply(InputIter beg,
                     InputSentinelIter end,
                     Callable&&... funcs)
{
    _impl::apply_concepts_check<InputIter, InputSentinelIter, Callable...>(*beg);

    for (auto _it = beg; _it != end; ++_it)
        std::visit(utility::overloaded { std::forward<Callable>(funcs)...,
                                         [](...){} }, *_it);
};


namespace _impl
{

template <
           typename InputIter,
           typename InputSentinelIter,
           typename OutputIter,
           typename... Callable,
           typename... VariantTypes
         >
requires(_concepts::IsInputIterator<InputIter>,
         _concepts::IsInputIterator<InputSentinelIter>,
         _concepts::IsOutputIterator<OutputIter>)
constexpr void filter_concepts_check(std::variant<VariantTypes...>)
{
    static_assert((... && std::is_copy_constructible_v<VariantTypes>),
                  "Variant types must be copy-constructible");

    static_assert((... && std::is_copy_assignable_v<VariantTypes>),
                  "Variant types must be copy-assignable");

    static_assert((... && std::is_destructible_v<VariantTypes>),
                  "Variant types must be destructible");

    static_assert(std::is_same_v < typename std::iterator_traits<InputIter>::value_type,
                                   typename std::iterator_traits<InputSentinelIter>::value_type >,
                  "Underlying types of begin and end iterator is not same");

    static_assert(std::is_same_v < typename std::iterator_traits<InputIter>::value_type,
                                   std::variant<VariantTypes...> >,
                  "Underlying type of output iterator must be the same with input iterators");

    static_assert(_concepts::IsComparable < InputIter, InputSentinelIter >,
                  "Iterators is not comparable");

    static_assert((... && IsCallableWithAnyOf<Callable, VariantTypes...>),
                  "One of passed functors not callable with all of variant types");
}

}

template <
           typename InputIter,
           typename InputSentinelIter,
           typename OutputIter,
           typename... Callable
         >
constexpr void filter(InputIter beg,
                      InputSentinelIter end,
                      OutputIter out,
                      Callable&&... funcs)
{
    _impl::filter_concepts_check<InputIter, InputSentinelIter, OutputIter, Callable...>(*beg);

    for (auto _it = beg; _it != end; ++_it)
    {
        if (std::visit(utility::overloaded{ std::forward<std::remove_reference_t<Callable>>(funcs)...,
                                            [](...) { return false; } },
                       *_it))
            *out++ = *_it;
    }
};


namespace _impl
{

template <
           typename InputIter,
           typename InputSentinelIter,
           typename AccType,
           bool IsGeneric,
           typename... Callable
         >
struct reduce_impl;

template <
           typename InputIter,
           typename InputSentinelIter,
           typename AccType,
           typename... Callable
         >
struct reduce_impl < InputIter, InputSentinelIter, AccType, true, Callable... >
{
    constexpr auto operator()(InputIter beg, InputSentinelIter end,
                              AccType initial_acc, Callable&&... funcs)
    {
        auto [first_func] = std::tuple<Callable...> { std::forward<Callable>(funcs)... };

        for (auto _it = beg; _it != end; ++_it)
        {
            initial_acc = visit(utility::overloaded {
                                    [&initial_acc, &first_func] (auto&& arg) { return first_func(initial_acc,
                                                                                                 std::forward<decltype(arg)>(arg)); } },
                                *_it);
        }
        return initial_acc;
    }
};

template <
           typename InputIter,
           typename InputSentinelIter,
           typename AccType,
           typename... Callable
         >
struct reduce_impl < InputIter, InputSentinelIter, AccType, false, Callable... >
{
    constexpr auto operator()(InputIter beg, InputSentinelIter end,
                              AccType initial_acc, Callable&&... funcs)
    {
        for (auto _it = beg; _it != end; ++_it)
        {
            initial_acc = std::visit(utility::make_overloaded_from_tup(
                utility::tup_funcs(initial_acc,
                                   std::forward<Callable>(funcs)...),
                std::make_index_sequence<sizeof...(Callable)>{},
                [&initial_acc](...) { return initial_acc; } ),
                                     *_it);
        }
        return initial_acc;
    }
};

template <
           typename InputIter,
           typename InputIterSentinel,
           typename AccType,
           typename... UnderlyingTypes,
           typename... Callable
         >
constexpr auto reduce_helper(InputIter beg, InputIterSentinel end,
                             AccType& acc, std::variant<UnderlyingTypes...>,
                             Callable&&... funcs)
{
    return reduce_impl <
                         InputIter, InputIterSentinel, AccType,
                         (... && utility::is_generic_2nd<Callable, AccType, UnderlyingTypes...>()),
                         Callable...
                       > {}
        (beg, end, acc, std::forward<Callable>(funcs)...);
};


template <typename F, typename Head, typename... Tail>
constexpr bool IsAnyFuncCallableForReduce = (... || _concepts::IsCallableWithReduce<F, Head, Tail>);

template <
           typename InputIter,
           typename InputSentinelIter,
           typename AccType,
           typename... Callable,
           typename... VariantTypes
         >
requires(_concepts::IsInputIterator<InputIter>,
         _concepts::IsInputIterator<InputSentinelIter>)
constexpr void reduce_concepts_check(std::variant<VariantTypes...>)
{
    static_assert((... && std::is_copy_constructible_v<VariantTypes>),
                  "Variant types must be copy-constructible");

    static_assert((... && std::is_copy_assignable_v<VariantTypes>),
                  "Variant types must be copy-assignable");

    static_assert((... && std::is_destructible_v<VariantTypes>),
                  "Variant types must be destructible");

    static_assert(std::is_same_v < typename std::iterator_traits<InputIter>::value_type,
                                   typename std::iterator_traits<InputSentinelIter>::value_type >,
                  "Underlying types of begin and end iterator is not same");

    static_assert(_concepts::IsComparable < InputIter, InputSentinelIter >,
                  "Iterators is not comparable");

    static_assert((... && IsAnyFuncCallableForReduce<Callable, AccType, VariantTypes...>),
                  "One of passed functors not callable with all of variant types "
                  "or not return accumulator value");
}

}

template <
           typename InputIter,
           typename InputIterSentinel,
           typename AccType,
           typename... Callable
         >
constexpr auto reduce(InputIter beg, InputIterSentinel end,
                      AccType acc, Callable&&... funcs)
{
    _impl::reduce_concepts_check<InputIter, InputIterSentinel, AccType, Callable...>(*beg);
    return _impl::reduce_helper(beg, end, acc,
                                *beg, std::forward<Callable>(funcs)...);
};


namespace _impl
{

template <
           typename InputIter,
           typename InputIterSentinel,
           typename OutputIter,
           bool IsGeneric,
           typename... Callable
         >
struct map_impl;

template <
           typename InputIter,
           typename InputIterSentinel,
           typename OutputIter,
           typename... Callable
         >
struct map_impl<InputIter, InputIterSentinel, OutputIter, true, Callable...>
{
    constexpr void operator()(InputIter beg, InputIterSentinel end,
                              OutputIter out, Callable&&... func) {
        for (auto _it = beg; _it != end; ++_it) {
            auto value = std::visit(std::forward<Callable>(func)..., *_it);
            *out++ = value;
        }
    }
};

template <
           typename InputIter,
           typename InputIterSentinel,
           typename OutputIter,
           typename... Callable
         >
struct map_impl<InputIter, InputIterSentinel, OutputIter, false, Callable...>
{
    constexpr void operator()(InputIter beg, InputIterSentinel end,
                              OutputIter out, Callable &&... funcs) {
        using value_type = std::remove_reference_t<decltype(*beg)>;
        using value_optional_type = std::optional<value_type>;

        for (auto _it = beg; _it != end; ++_it) {

            auto maybe_value = std::visit(utility::make_overloaded_from_tup(
                utility::tup_funcs_opt(std::forward<Callable>(funcs)...),
                std::make_index_sequence<sizeof...(Callable)>{},
                [](...) { return value_optional_type {}; }),
                                          *_it);
            if (maybe_value)
                *out++ = maybe_value.value();
        }
    }
};

template <typename F, typename Out, typename... Types>
constexpr bool IsAnyFuncCallableForMap = (... || _concepts::IsCallableWithMap<F, Types, Out>);

template <
           typename InputIter,
           typename InputSentinelIter,
           typename OutputIter,
           typename... Callable,
           typename... VariantTypes
         >
requires(_concepts::IsInputIterator<InputIter>,
         _concepts::IsInputIterator<InputSentinelIter>,
         _concepts::IsOutputIterator<OutputIter>)
constexpr void map_concepts_check(std::variant<VariantTypes...>)
{
    using output_type = std::remove_reference_t<decltype(*(std::declval<OutputIter>()))>;

    static_assert((... && std::is_copy_constructible_v<VariantTypes>),
                  "Variant types must be copy-constructible");

    static_assert((... && std::is_copy_assignable_v<VariantTypes>),
                  "Variant types must be copy-assignable");

    static_assert((... && std::is_destructible_v<VariantTypes>),
                  "Variant types must be destructible");

    static_assert(std::is_same_v < typename std::iterator_traits<InputIter>::value_type,
                                   typename std::iterator_traits<InputSentinelIter>::value_type >,
                  "Underlying types of begin and end iterator is not same");

    static_assert(_concepts::IsComparable < InputIter, InputSentinelIter >,
                  "Iterators is not comparable");

    static_assert((... && IsAnyFuncCallableForMap<Callable, output_type, VariantTypes...>),
                  "One of passed functors not callable with all of variant types "
                  "or not return valid typed value");
}

}

template <
           typename InputIter,
           typename InputIterSentinel,
           typename OutputIter,
           typename... UnderlyingTypes,
           typename... Callable
         >
constexpr void map_helper(InputIter beg, InputIterSentinel end,
                          OutputIter out, std::variant<UnderlyingTypes...>,
                          Callable&&... funcs)
{
    _impl::map_impl <
                      InputIter, InputIterSentinel, OutputIter,
                      (... && utility::is_generic<Callable, UnderlyingTypes...>()),
                      Callable...
                    > {}
    (beg, end, out, std::forward<Callable>(funcs)...);
};

template <
           typename InputIter,
           typename InputIterSentinel,
           typename OutputIter,
           typename... Callable
         >
constexpr void map(InputIter beg, InputIterSentinel end,
                   OutputIter out, Callable&&... funcs)
{
    _impl::map_concepts_check<InputIter, InputIterSentinel, OutputIter, Callable...>(*beg);
    map_helper(beg, end, out, *beg, std::forward<Callable>(funcs)...);
};

}

#endif //POLYMORPHISMTESTS_FUNCS_HPP
