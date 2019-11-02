#pragma once

template<typename F, typename... Args>
struct is_invocable
{
    template<class U> static auto test(U* p) -> decltype((*p)(std::declval<Args>()...), void(), std::true_type());
    template<class U> static auto test(...) -> decltype(std::false_type());

    typedef decltype(test<F>(0)) type;
    static constexpr bool value = decltype(test<F>(0))::value;
};