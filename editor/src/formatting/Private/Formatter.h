/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once

#include <SFML/Graphics.hpp>

#include <unordered_set>

#include "graph.hpp"

struct FFormatter
{
    void SetZoomLevelTo11Scale() const;
    void RestoreZoomLevel() const;
    
    bool PreCommand();
    void Translate(std::unordered_set<NodeId> Nodes, sf::Vector2f Offset) const;
    void Format();
    void PlaceBlock();
};
