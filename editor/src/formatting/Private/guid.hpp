#pragma once

#include <array>
#include <random>

using FGuid = std::array<std::uint32_t, 4>;

template<typename T> void hash_combine(size_t& seed, T const& v)
{
    seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<>
struct std::hash<FGuid>
{
    std::size_t operator()(const FGuid& s) const noexcept
    {
        auto h = std::hash<std::uint32_t>{}(s[0]);
        hash_combine(h, s[1]);
        hash_combine(h, s[2]);
        hash_combine(h, s[3]);
        return h;
    }
};

inline FGuid NewGuid()
{
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dis(0, static_cast<uint32_t>(-1));
    return { dis(gen), dis(gen), dis(gen), dis(gen) };
}