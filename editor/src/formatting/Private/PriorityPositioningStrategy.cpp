/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "PriorityPositioningStrategy.h"
#include "FormatterGraph.h"
#include "FormatterSettings.h"
#include "EvenlyPlaceStrategy.h"

#include "RectHelper.hpp"

#include <algorithm>

static sf::FloatRect PlaceNodeInLayer(std::vector<FFormatterNode*>& Layer)
{
    sf::Vector2f Position{};
    sf::Vector2f Max{};
    bool first = true;
    for (auto Node : Layer)
    {
        Node->SetPosition(Position);
        Max.x = std::max(Max.x, Position.x + Node->Size.x);
        Max.y = std::max(Max.y, Position.y + Node->Size.y);
        Position.y += Node->Size.y;
    }
    return { {}, Max };
}

static bool GetBarycenter(FFormatterNode* Node, EFormatterPinDirection Direction, float& Barycenter)
{
    float ToLayerYSum = 0.0f;
    float SelfYSum = 0.0f;
    const auto& Edges = Direction == EFormatterPinDirection::Out ? Node->OutEdges : Node->InEdges;
    if (Edges.size() == 0)
    {
        Barycenter = Node->GetPosition().y;
        return false;
    }
    for (auto Edge : Edges)
    {
        const auto LinkedToNode = Edge->To->OwningNode;
        const auto LinkedToPosition = LinkedToNode->GetPosition() + Edge->To->NodeOffset;
        ToLayerYSum += LinkedToPosition.y;
        SelfYSum += Edge->From->NodeOffset.y;
    }
    const float ToAverage = ToLayerYSum / Edges.size();
    const float FromAverage = SelfYSum / Edges.size();
    Barycenter = ToAverage - FromAverage;
    return true;
}

static bool GetClosestPositionToBarycenter(const std::vector<FFormatterNode*>& Slots, std::int32_t Previous, std::int32_t Next, FFormatterNode* Node, float& Barycenter, float& Y)
{
    const UFormatterSettings& Settings = UFormatterSettings::Get();
    FFormatterNode* PreviousNode = nullptr;
    bool bUpFree = false;
    if (Previous < 0 || Slots[Previous] == nullptr)
    {
        bUpFree = true;
    }
    else
    {
        PreviousNode = Slots[Previous];
    }
    FFormatterNode* NextNode = nullptr;
    bool bDownFree = false;
    if (Next >= Slots.size() || Slots[Next] == nullptr)
    {
        bDownFree = true;
    }
    else
    {
        NextNode = Slots[Next];
    }
    if (bUpFree && bDownFree)
    {
        Y = Barycenter;
        return true;
    }
    if (bUpFree)
    {
        const float MostDown = NextNode->GetPosition().y - Node->Size.y - Settings.VerticalSpacing;
        Y = std::min(Barycenter, MostDown);
        return true;
    }
    if (bDownFree)
    {
        const float MostUp = PreviousNode->GetPosition().y + PreviousNode->Size.y + Settings.VerticalSpacing;
        Y = std::max(MostUp, Barycenter);
        return true;
    }
    const float MostDown = NextNode->GetPosition().y - Node->Size.y - Settings.VerticalSpacing;
    const float MostUp = PreviousNode->GetPosition().y + PreviousNode->Size.y + Settings.VerticalSpacing;
    if (MostDown < MostUp)
    {
        Y = MostUp - MostDown;
        Barycenter = MostDown + Y / 2.0f;
        return false;
    }
    Y = std::clamp(Barycenter, MostUp, MostDown);
    return true;
}

static void ShiftInLayer(std::vector<FFormatterNode*> Slots, std::int32_t Index, float Distance)
{
    for (std::int32_t i = 0; i < Index; i++)
    {
        auto Node = Slots[i];
        if (Node != nullptr)
        {
            Node->SetPosition(Node->GetPosition() - sf::Vector2f(0, Distance));
        }
    }
    for (std::int32_t i = Index + 1; i < Slots.size(); i++)
    {
        auto Node = Slots[i];
        if (Node != nullptr)
        {
            Node->SetPosition(Node->GetPosition() + sf::Vector2f(0, Distance));
        }
    }
}

static void PositioningSweep(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes, EFormatterPinDirection Direction, const std::vector<sf::FloatRect>& LayersBound)
{
    std::int32_t StartIndex, EndIndex, Step;
    if (Direction == EFormatterPinDirection::In)
    {
        StartIndex = 1;
        EndIndex = InLayeredNodes.size();
        Step = 1;
    }
    else
    {
        StartIndex = InLayeredNodes.size() - 2;
        EndIndex = -1;
        Step = -1;
    }
    for (std::int32_t i = StartIndex; i != EndIndex; i += Step)
    {
        auto CurrentLayer = InLayeredNodes[i];
        auto PreviousLayer = InLayeredNodes[i - Step];
        std::vector<FFormatterNode*> Slots;
        Slots.resize(CurrentLayer.size());
        std::vector<FFormatterNode*> PriorityList = CurrentLayer;
        for (auto Node : CurrentLayer)
        {
            Node->PositioningPriority = Node->CalcPriority(Direction);
        }
        std::ranges::sort(PriorityList, [](const FFormatterNode* A, const FFormatterNode* B)
        {
            return A->PositioningPriority > B->PositioningPriority;
        });
        sf::Vector2f Position;
        for (auto Node : PriorityList)
        {
            float Barycenter;
            const bool IsConnected = GetBarycenter(Node, Direction, Barycenter);
            if (IsConnected)
            {
                Position = LayersBound[i].position;
            }
            else
            {
                Position = LayersBound[i].position + LayersBound[i].size - sf::Vector2f(Node->Size.x, 0);
            }

            const std::int32_t Index = std::distance(CurrentLayer.begin(), std::ranges::find(CurrentLayer, Node));
            const std::int32_t Previous = Index - 1;
            const std::int32_t Next = Index + 1;
            float OutY;
            if (GetClosestPositionToBarycenter(Slots, Previous, Next, Node, Barycenter, OutY))
            {
                Position.y = OutY;
            }
            else
            {
                ShiftInLayer(Slots, Index, OutY / 2.0f);
                Position.y = Barycenter;
            }
            Slots[Index] = Node;
            Node->SetPosition(Position);
        }
    }
}

FPriorityPositioningStrategy::FPriorityPositioningStrategy(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes)
    : IPositioningStrategy(InLayeredNodes)
{
    if (InLayeredNodes.size() < 2)
    {
        FEvenlyPlaceStrategy LeftToRightPositioningStrategy(InLayeredNodes);
        TotalBound = LeftToRightPositioningStrategy.GetTotalBound();
        return;
    }
    assert(InLayeredNodes[0].size() > 0);
    FFormatterNode* FirstNode = InLayeredNodes[0][0];
    const sf::Vector2f OldPosition = FirstNode->GetPosition();
    const auto LayersBound = FFormatterGraph::CalculateLayersBound(InLayeredNodes);
    for (auto& Layer : InLayeredNodes)
    {
        PlaceNodeInLayer(Layer);
    }
    PositioningSweep(InLayeredNodes, EFormatterPinDirection::In, LayersBound);
    PositioningSweep(InLayeredNodes, EFormatterPinDirection::Out, LayersBound);
    PositioningSweep(InLayeredNodes, EFormatterPinDirection::In, LayersBound);

    std::optional<sf::FloatRect> Bound;
    const sf::Vector2f NewPosition = FirstNode->GetPosition();
    for (std::int32_t i = 0; i < InLayeredNodes.size(); i++)
    {
        const sf::Vector2f Offset = OldPosition - NewPosition;
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
