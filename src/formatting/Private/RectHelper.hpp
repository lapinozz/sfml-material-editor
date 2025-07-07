#pragma once

#include <SFML/Graphics.hpp>

template<typename T>
sf::Rect<T> mergeRects(const sf::Rect<T>& r1, const sf::Rect<T>& r2)
{
    const auto minX = std::min(r1.position.x, r2.position.x);
    const auto minY = std::min(r1.position.y, r2.position.y);
    const auto maxX = std::max(r1.position.x + r1.size.x, r2.position.x + r2.size.x);
    const auto maxY = std::max(r1.position.y + r1.size.y, r2.position.y + r2.size.y);

    return { {minX, minY}, {maxX - minX, maxY - minY} };
}