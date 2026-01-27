/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "FastAndSimplePositioningStrategy.h"
#include "FormatterGraph.h"
#include "FormatterSettings.h"

#include "RectHelper.hpp"

void FFastAndSimplePositioningStrategy::Initialize()
{
    for (auto& Layer : LayeredNodes)
    {
        for (std::int32_t i = 0; i < Layer.size(); i++)
        {
            PosMap.emplace(Layer[i], i);
            if (i != 0)
            {
                PredecessorMap.emplace(Layer[i], Layer[i - 1]);
            }
            else
            {
                PredecessorMap.emplace(Layer[i], nullptr);
            }
            if (i != Layer.size() - 1)
            {
                SuccessorMap.emplace(Layer[i], Layer[i + 1]);
            }
            else
            {
                SuccessorMap.emplace(Layer[i], nullptr);
            }
        }
    }
    MarkConflicts();
}

void FFastAndSimplePositioningStrategy::MarkConflicts()
{
    for (std::int32_t i = 1; i < LayeredNodes.size() - 1; i++)
    {
        std::int32_t k0 = 0;
        std::int32_t l = 1;
        for (std::int32_t l1 = 0; l1 < LayeredNodes[i + 1].size(); l1++)
        {
            auto Node = LayeredNodes[i + 1][l1];
            bool IsCrossingInnerSegment = Node->IsCrossingInnerSegment(LayeredNodes[i + 1], LayeredNodes[i]);
            if (l1 == LayeredNodes[i + 1].size() - 1 || IsCrossingInnerSegment)
            {
                std::int32_t k1 = LayeredNodes[i].size();
                if (IsCrossingInnerSegment)
                {
                    const auto MedianUpper = Node->GetMedianUpper();
                    k1 = PosMap[MedianUpper];
                }
                while (l < l1)
                {
                    auto UpperNodes = Node->GetUppers();
                    for (auto UpperNode : UpperNodes)
                    {
                        auto k = PosMap[UpperNode];
                        if (k < k0 || k > k1)
                        {
                            ConflictMarks.emplace(UpperNode, Node);
                        }
                    }
                    ++l;
                }
                k0 = k1;
            }
        }
    }
}

void FFastAndSimplePositioningStrategy::DoVerticalAlignment()
{
    RootMap = {};
    AlignMap = {};
    for (auto Layer : LayeredNodes)
    {
        for (auto Node : Layer)
        {
            RootMap.emplace(Node, Node);
            AlignMap.emplace(Node, Node);
        }
    }
    std::int32_t LayerStep = IsUpperDirection ? 1 : -1;
    std::int32_t LayerStart = IsUpperDirection ? 0 : LayeredNodes.size() - 1;
    std::int32_t LayerEnd = IsUpperDirection ? LayeredNodes.size() : -1;
    for (std::int32_t i = LayerStart; i != LayerEnd; i += LayerStep)
    {
        float Weight = 0.0f;
        std::int32_t Guide = IsLeftDirection ? -1 : INT_MAX;
        std::int32_t Step = IsLeftDirection ? 1 : -1;
        std::int32_t Start = IsLeftDirection ? 0 : LayeredNodes[i].size() - 1;
        std::int32_t End = IsLeftDirection ? LayeredNodes[i].size() : -1;
        for (std::int32_t k = Start; k != End; k += Step)
        {
            auto Node = LayeredNodes[i][k];
            auto Adjacencies = IsUpperDirection ? Node->GetUppers() : Node->GetLowers();
            if (Adjacencies.size() > 0)
            {
                std::int32_t ma = std::trunc((Adjacencies.size() + 1) / 2.0f - 1);
                std::int32_t mb = std::ceil((Adjacencies.size() + 1) / 2.0f - 1);
                for (std::int32_t m = ma; m <= mb; m++)
                {
                    if (AlignMap[Node] == Node)
                    {
                        auto& MedianNode = Adjacencies[m];
                        bool IsMarked = ConflictMarks.contains(MedianNode) && ConflictMarks[MedianNode] == Node;
                        float MaxWeight = MedianNode->GetMaxWeight(IsUpperDirection ? EFormatterPinDirection::Out : EFormatterPinDirection::In);
                        float LinkWeight = Node->GetMaxWeightToNode(MedianNode, IsUpperDirection ? EFormatterPinDirection::In : EFormatterPinDirection::Out);
                        const auto MedianNodePos = PosMap[MedianNode];
                        bool IsGuideAccepted = IsLeftDirection ? MedianNodePos > Guide : MedianNodePos < Guide;
                        if (!IsMarked)
                        {
                            if (IsGuideAccepted && LinkWeight == MaxWeight)
                            {
                                AlignMap[MedianNode] = Node;
                                RootMap[Node] = RootMap[MedianNode];
                                AlignMap[Node] = RootMap[Node];
                                Guide = MedianNodePos;
                            }
                        }
                    }
                }
            }
        }
    }
}

void FFastAndSimplePositioningStrategy::DoHorizontalCompaction()
{
    SinkMap = {};
    ShiftMap = {};
    *XMap = {};
    for (auto& Layer : LayeredNodes)
    {
        for (auto Node : Layer)
        {
            SinkMap.emplace(Node, Node);
            if (IsLeftDirection)
            {
                ShiftMap.emplace(Node, FLT_MAX);
            }
            else
            {
                ShiftMap.emplace(Node, -FLT_MAX);
            }
            XMap->emplace(Node, NAN);
        }
    }
    for (auto& Layer : LayeredNodes)
    {
        for (auto Node : Layer)
        {
            if (RootMap[Node] == Node)
            {
                PlaceBlock(Node);
            }
        }
    }
    for (auto& Layer : LayeredNodes)
    {
        for (auto Node : Layer)
        {
            auto& RootNode = RootMap[Node];
            (*XMap)[Node] = (*XMap)[RootNode];
        }
    }
    for (auto& Layer : LayeredNodes)
    {
        for (auto Node : Layer)
        {
            auto& RootNode = RootMap[Node];
            const float Shift = ShiftMap[SinkMap[RootNode]];
            if ((IsLeftDirection && Shift < FLT_MAX) || (!IsLeftDirection && Shift > -FLT_MAX))
            {
                (*XMap)[Node] = (*XMap)[Node] + Shift;
            }
        }
    }
    for (auto& Layer : LayeredNodes)
    {
        for (auto Node : Layer)
        {
            (*XMap)[Node] += InnerShiftMap[Node];
        }
    }
}

void FFastAndSimplePositioningStrategy::PlaceBlock(FFormatterNode* BlockRoot)
{
    const UFormatterSettings& Settings = UFormatterSettings::Get();
    if (std::isnan((*XMap)[BlockRoot]))
    {
        bool Initial = true;
        (*XMap)[BlockRoot] = 0;
        auto Node = BlockRoot;
        do
        {
            const auto Adjacency = IsLeftDirection ? PredecessorMap[Node] : SuccessorMap[Node];
            if (Adjacency != nullptr)
            {
                float AdjacencyHeight, NodeHeight;
                float Spacing;
                if (IsHorizontalDirection)
                {
                    AdjacencyHeight = Adjacency->Size.y;
                    NodeHeight = Node->Size.y;
                    Spacing = Settings.VerticalSpacing;
                }
                else
                {
                    AdjacencyHeight = Adjacency->Size.x;
                    NodeHeight = Node->Size.x;
                    Spacing = Settings.HorizontalSpacing;
                }

                const auto PrevBlockRoot = RootMap[Adjacency];
                PlaceBlock(PrevBlockRoot);
                if (SinkMap[BlockRoot] == BlockRoot)
                {
                    SinkMap[BlockRoot] = SinkMap[PrevBlockRoot];
                }
                if (SinkMap[BlockRoot] != SinkMap[PrevBlockRoot])
                {
                    float LeftShift = (*XMap)[BlockRoot] - (*XMap)[PrevBlockRoot] + InnerShiftMap[Node] - InnerShiftMap[Adjacency] - AdjacencyHeight - Spacing;
                    float RightShift = (*XMap)[BlockRoot] - (*XMap)[PrevBlockRoot] - InnerShiftMap[Node] + InnerShiftMap[Adjacency] + NodeHeight + Spacing;
                    float Shift = IsLeftDirection ? std::min(ShiftMap[SinkMap[PrevBlockRoot]], LeftShift) : std::max(ShiftMap[SinkMap[PrevBlockRoot]], RightShift);
                    ShiftMap[SinkMap[PrevBlockRoot]] = Shift;
                }
                else
                {
                    float LeftShift = InnerShiftMap[Adjacency] + AdjacencyHeight - InnerShiftMap[Node] + Spacing;
                    float RightShift = -NodeHeight - Spacing + InnerShiftMap[Adjacency] - InnerShiftMap[Node];
                    float Shift = IsLeftDirection ? LeftShift : RightShift;
                    float Position = (*XMap)[PrevBlockRoot] + Shift;
                    if (Initial)
                    {
                        (*XMap)[BlockRoot] = Position;
                        Initial = false;
                    }
                    else
                    {
                        Position = IsLeftDirection ? std::max((*XMap)[BlockRoot], Position) : std::min((*XMap)[BlockRoot], Position);
                        (*XMap)[BlockRoot] = Position;
                    }
                }
            }
            Node = AlignMap[Node];
        }
        while (Node != BlockRoot);
    }
}

void FFastAndSimplePositioningStrategy::CalculateInnerShift()
{
    InnerShiftMap = {};
    BlockWidthMap = {};
    for (auto& Layer : LayeredNodes)
    {
        for (auto Node : Layer)
        {
            if (RootMap[Node] == Node)
            {
                InnerShiftMap.emplace(Node, 0);
                float Left = 0, Right = IsHorizontalDirection ? Node->Size.y : Node->Size.x;
                auto RootNode = Node;
                auto UpperNode = Node;
                auto LowerNode = AlignMap[RootNode];
                while (true)
                {
                    const float UpperPosition = UpperNode->GetLinkedPositionToNode(LowerNode, IsUpperDirection ? EFormatterPinDirection::Out : EFormatterPinDirection::In, IsHorizontalDirection);
                    const float LowerPosition = LowerNode->GetLinkedPositionToNode(UpperNode, IsUpperDirection ? EFormatterPinDirection::In : EFormatterPinDirection::Out, IsHorizontalDirection);
                    const float Shift = InnerShiftMap[UpperNode] + UpperPosition - LowerPosition;
                    InnerShiftMap[LowerNode] = Shift;
                    Left = std::min(Left, Shift);
                    Right = std::max(Right, Shift + (IsHorizontalDirection ? LowerNode->Size.y : LowerNode->Size.x));
                    UpperNode = LowerNode;
                    LowerNode = AlignMap[UpperNode];
                    if (LowerNode == RootNode)
                    {
                        break;
                    }
                }
                auto CheckNode = Node;
                do
                {
                    InnerShiftMap[CheckNode] -= Left;
                    CheckNode = AlignMap[CheckNode];
                }
                while (CheckNode != Node);
                BlockWidthMap[Node] = Right - Left;
            }
        }
    }
}

void FFastAndSimplePositioningStrategy::Sweep()
{
    IsUpperDirection = true;
    IsLeftDirection = true;
    XMap = &UpperLeftPositionMap;
    DoOnePass();

    IsUpperDirection = true;
    IsLeftDirection = false;
    XMap = &UpperRightPositionMap;
    DoOnePass();

    IsUpperDirection = false;
    IsLeftDirection = true;
    XMap = &LowerLeftPositionMap;
    DoOnePass();

    IsUpperDirection = false;
    IsLeftDirection = false;
    XMap = &LowerRightPositionMap;
    DoOnePass();

    Combine();
}

void FFastAndSimplePositioningStrategy::Combine()
{
    std::vector<std::unordered_map<FFormatterNode*, float>> Layouts = {UpperLeftPositionMap, UpperRightPositionMap, LowerLeftPositionMap, LowerRightPositionMap};
    std::vector<std::tuple<float, float>> Bounds;
    Bounds.resize(Layouts.size());
    std::int32_t MinWidthIndex = -1;
    float MinWidth = FLT_MAX;
    for (std::int32_t i = 0; i < Layouts.size(); i++)
    {
        auto& Layout = Layouts[i];
        float LeftMost = FLT_MAX, RightMost = -FLT_MAX;
        for (auto& Pair : Layout)
        {
            if (Pair.second < LeftMost)
            {
                LeftMost = Pair.second;
            }
            if (Pair.second > RightMost)
            {
                RightMost = Pair.second;
            }
        }
        if (RightMost - LeftMost < MinWidth)
        {
            MinWidth = RightMost - LeftMost;
            MinWidthIndex = i;
        }
        Bounds[i] = std::tuple<float, float>(LeftMost, RightMost);
    }
    for (std::int32_t i = 0; i < Layouts.size(); i++)
    {
        if (i != MinWidthIndex)
        {
            float Offset = std::get<0>(Bounds[MinWidthIndex]) - std::get<0>(Bounds[i]);
            for (auto& Pair : Layouts[i])
            {
                Pair.second += Offset;
            }
        }
    }
    const UFormatterSettings& Settings = UFormatterSettings::Get();
    for (auto& Layer : LayeredNodes)
    {
        for (auto Node : Layer)
        {
            std::vector Values = {Layouts[0][Node], Layouts[1][Node], Layouts[2][Node], Layouts[3][Node]};
            std::ranges::sort(Values);
            if (!IsHorizontalDirection)
            {
                CombinedPositionMap.emplace(Node, (Values[0] + Values[3]) / 2.0f);
            }
            else
            {
                if (Settings.PositioningAlgorithm == EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodTop)
                {
                    CombinedPositionMap.emplace(Node, Values[MinWidthIndex]);
                }
                else if (Settings.PositioningAlgorithm == EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodMedian)
                {
                    CombinedPositionMap.emplace(Node, (Values[2] + Values[3]) / 2.0f);
                }
            }
        }
    }
    XMap = &CombinedPositionMap;
}

void FFastAndSimplePositioningStrategy::DoOnePass()
{
    DoVerticalAlignment();
    CalculateInnerShift();
    DoHorizontalCompaction();
}

FFastAndSimplePositioningStrategy::FFastAndSimplePositioningStrategy(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes, bool IsHorizontalDirection)
    : IPositioningStrategy(InLayeredNodes)
    , IsHorizontalDirection(IsHorizontalDirection)
{
    const auto LayersBound = FFormatterGraph::CalculateLayersBound(InLayeredNodes, IsHorizontalDirection);
    FFormatterNode* FirstNode = InLayeredNodes[0][0];
    const sf::Vector2f OldPosition = FirstNode->GetPosition();
    Initialize();
    Sweep();
    for (int i = 0; i < LayeredNodes.size(); i++)
    {
        auto& Layer = LayeredNodes[i];
        for (auto Node : Layer)
        {
            float X, Y;
            float *pX, *pY;
            if (IsHorizontalDirection)
            {
                pX = &X;
                pY = &Y;
            }
            else
            {
                pX = &Y;
                pY = &X;
            }
            if (Node->InEdges.size() == 0)
            {
                if (IsHorizontalDirection)
                {
                    *pX = LayersBound[i].position.x + LayersBound[i].size.x - Node->Size.x;
                }
                else
                {
                    *pX = LayersBound[i].position.y + LayersBound[i].size.y - Node->Size.y;
                }
            }
            else
            {
                if (IsHorizontalDirection)
                {
                    *pX = LayersBound[i].position.x;
                }
                else
                {
                    *pX = LayersBound[i].position.y;
                }
            }
            *pY = (*XMap)[Node];
            Node->SetPosition(sf::Vector2f(X, Y));
        }
    }
    const sf::Vector2f NewPosition = FirstNode->GetPosition();
    const sf::Vector2f Offset = OldPosition - NewPosition;
    std::optional<sf::FloatRect> Bound;
    for (std::int32_t i = 0; i < InLayeredNodes.size(); i++)
    {
        for (auto Node : InLayeredNodes[i])
        {
            Node->SetPosition(Node->GetPosition() + Offset);
            if (Bound)
            {
                *Bound = mergeRects(*Bound, sf::FloatRect(Node->GetPosition(), Node->Size));
            }
            else
            {
                Bound = sf::FloatRect(Node->GetPosition(), Node->Size);
            }
        }
    }
    TotalBound = Bound;
}
