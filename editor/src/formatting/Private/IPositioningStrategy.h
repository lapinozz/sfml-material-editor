/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once

#include <SFML/Graphics/Rect.hpp>

#include <vector>
#include <optional>

class FFormatterNode;

class IPositioningStrategy
{
protected:
    std::optional<sf::FloatRect> TotalBound{};
    std::vector<std::vector<FFormatterNode*>> LayeredNodes;
public:
    explicit IPositioningStrategy(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes)
        : LayeredNodes(InLayeredNodes)
    {
    }

    virtual ~IPositioningStrategy() = default;
    sf::FloatRect GetTotalBound() const { return *TotalBound; }
};
