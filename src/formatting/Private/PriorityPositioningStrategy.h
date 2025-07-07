/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once

#include "IPositioningStrategy.h"

#include <vector>

class FPriorityPositioningStrategy : public IPositioningStrategy
{
public:
    explicit FPriorityPositioningStrategy(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes);
};
