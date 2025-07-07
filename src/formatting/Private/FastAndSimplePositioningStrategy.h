/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once

#include "IPositioningStrategy.h"

#include <unordered_map>

class FFastAndSimplePositioningStrategy : public IPositioningStrategy
{
    std::unordered_map<FFormatterNode*, FFormatterNode*> ConflictMarks;
    std::unordered_map<FFormatterNode*, FFormatterNode*> RootMap;
    std::unordered_map<FFormatterNode*, FFormatterNode*> AlignMap;
    std::unordered_map<FFormatterNode*, FFormatterNode*> SinkMap;
    std::unordered_map<FFormatterNode*, float> ShiftMap;
    std::unordered_map<FFormatterNode*, float> InnerShiftMap;
    std::unordered_map<FFormatterNode*, float>* XMap;
    std::unordered_map<FFormatterNode*, std::int32_t> PosMap;
    std::unordered_map<FFormatterNode*, float> BlockWidthMap;
    std::unordered_map<FFormatterNode*, FFormatterNode*> PredecessorMap;
    std::unordered_map<FFormatterNode*, FFormatterNode*> SuccessorMap;
    std::unordered_map<FFormatterNode*, float> UpperLeftPositionMap;
    std::unordered_map<FFormatterNode*, float> UpperRightPositionMap;
    std::unordered_map<FFormatterNode*, float> LowerLeftPositionMap;
    std::unordered_map<FFormatterNode*, float> LowerRightPositionMap;
    std::unordered_map<FFormatterNode*, float> CombinedPositionMap;
    bool IsUpperDirection;
    bool IsLeftDirection;
    bool IsHorizontalDirection;
    void Initialize();
    void MarkConflicts();
    void DoVerticalAlignment();
    void DoHorizontalCompaction();
    void PlaceBlock(FFormatterNode* BlockRoot);
    void CalculateInnerShift();
    void Sweep();
    void Combine();
    void DoOnePass();
public:
    explicit FFastAndSimplePositioningStrategy(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes, bool IsHorizontalDirection = true);
};
