/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "EvenlyPlaceStrategy.h"
#include "FormatterGraph.h"
#include "FormatterSettings.h"

#include "RectHelper.hpp"

sf::FloatRect FEvenlyPlaceStrategy::PlaceNodeInLayer(std::vector<FFormatterNode*>& Layer, const sf::FloatRect* PreBound)
{
    std::optional<sf::FloatRect> Bound;
    const UFormatterSettings& Settings = UFormatterSettings::Get();
    sf::Vector2f Position{};
    if (PreBound)
    {
        Position = sf::Vector2f(PreBound->position.x + PreBound->size.x + Settings.HorizontalSpacing, 0);
    }
    else
    {
        Position = sf::Vector2f(0, 0);
    }
    for (auto Node : Layer)
    {
        if (!Node->OriginalNode)
        {
            Node->SetPosition(Position);
            continue;
        }
        Node->SetPosition(Position);
        if (Bound)
        {
            *Bound = mergeRects(*Bound, sf::FloatRect(Position, Node->Size));
        }
        else
        {
            Bound = sf::FloatRect(Position, Node->Size);
        }
        Position.y += Node->Size.y + Settings.VerticalSpacing;
    }
    return *Bound;
}

FFormatterNode* FEvenlyPlaceStrategy::FindFirstNodeInLayeredList(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes)
{
    for (const auto& Layer : InLayeredNodes)
    {
        for (auto Node : Layer)
        {
            return Node;
        }
    }
    return nullptr;
}

FEvenlyPlaceStrategy::FEvenlyPlaceStrategy(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes)
    : IPositioningStrategy(InLayeredNodes)
{
    sf::Vector2f StartPosition;
    FFormatterNode* FirstNode = FindFirstNodeInLayeredList(InLayeredNodes);
    if (FirstNode != nullptr)
    {
        StartPosition = FirstNode->GetPosition();
    }

    float MaxHeight = 0;
    std::optional<sf::FloatRect> PreBound;
    std::vector<sf::FloatRect> Bounds;
    for (auto& Layer : InLayeredNodes)
    {
        PreBound = PlaceNodeInLayer(Layer, PreBound ? &*PreBound : nullptr);
        Bounds.push_back(*PreBound);
        if (TotalBound)
        {
            *TotalBound = mergeRects(*TotalBound, *PreBound);
        }
        else
        {
            TotalBound = PreBound;
        }
        const float Height = PreBound->size.y;
        if (Height > MaxHeight)
        {
            MaxHeight = Height;
        }
    }

    StartPosition -= sf::Vector2f(0, MaxHeight - Bounds[0].size.y) / 2.0f;

    for (std::int32_t i = 0; i < InLayeredNodes.size(); i++)
    {
        const sf::Vector2f Offset = sf::Vector2f(0, (MaxHeight - Bounds[i].size.y) / 2.0f) + StartPosition;
        for (auto Node : InLayeredNodes[i])
        {
            Node->SetPosition(Node->GetPosition() + Offset);
        }
    }
    TotalBound = sf::FloatRect(StartPosition, TotalBound->size);
}
