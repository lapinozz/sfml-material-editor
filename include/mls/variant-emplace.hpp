#pragma once

#include <variant>
#include <assert.h>

template <typename... Ts>
void variantEmplace(std::variant<Ts...>& v, std::size_t i)
{
    assert(i < sizeof...(Ts));
    static constexpr std::variant<Ts...> table[] = {Ts{}...};
    v = table[i];
}