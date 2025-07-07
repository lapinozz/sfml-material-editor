/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once

#include "FormatterGraph.h"

#include <unordered_map>

class UFormatterSettings
{
public:
    UFormatterSettings();

    static UFormatterSettings& Get();

    /** Positioning algorithm*/
    EGraphFormatterPositioningAlgorithm PositioningAlgorithm;

    /** Spacing between two layers */
    std::int32_t HorizontalSpacing;

    /** Spacing between two nodes */
    std::int32_t VerticalSpacing;

    /** Maximum number of nodes per layer, 0 indicates no restriction */
    std::int32_t MaxLayerNodes;

    /** Vertex ordering max iterations */
    std::int32_t MaxOrderingIterations;

    /** Straight connections old settings */
    sf::Vector2f ForwardSplineTangentFromHorizontalDelta;
    sf::Vector2f ForwardSplineTangentFromVerticalDelta;
    sf::Vector2f BackwardSplineTangentFromHorizontalDelta;
    sf::Vector2f BackwardSplineTangentFromVerticalDelta;
};
