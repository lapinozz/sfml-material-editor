/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once

#include "IPositioningStrategy.h"

#include <SFML/Graphics/Rect.hpp>

class FEvenlyPlaceStrategy : public IPositioningStrategy
{
    sf::FloatRect PlaceNodeInLayer(std::vector<FFormatterNode*>& Layer, const sf::FloatRect* PreBound);
    FFormatterNode* FindFirstNodeInLayeredList(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes);
public:
    explicit FEvenlyPlaceStrategy(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes);
};
