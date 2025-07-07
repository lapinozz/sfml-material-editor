/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "Formatter.h"
#include "FormatterGraph.h"
#include "FormatterSettings.h"
#include "UEGraphAdapter.h"

#include "formatting/graph_layout/graph_layout.h"
using namespace graph_layout;

bool FFormatter::PreCommand()
{
    const UFormatterSettings* Settings = &UFormatterSettings::Get();
    FFormatterGraph::HorizontalSpacing = Settings->HorizontalSpacing;
    FFormatterGraph::VerticalSpacing = Settings->VerticalSpacing;
//    FFormatterGraph::SpacingFactorOfGroup = Settings->SpacingFactorOfParameterGroup;
    FFormatterGraph::MaxLayerNodes = Settings->MaxLayerNodes;
    FFormatterGraph::MaxOrderingIterations = Settings->MaxOrderingIterations;
    FFormatterGraph::PositioningAlgorithm = Settings->PositioningAlgorithm;
    return true;
}

void FFormatter::Translate(std::unordered_set<NodeId> Nodes, sf::Vector2f Offset) const
{
    if (Offset.x == 0 && Offset.y == 0)
    {
        return;
    }
    for (auto Node : Nodes)
    {
        ed::SetNodePosition(Node.Get(), ed::GetNodePosition(Node.Get()) + ImVec2{Offset.x, Offset.y});
    }
}

static std::unordered_set<NodeId> GetSelectedNodes()
{
    std::unordered_set<NodeId> SelectedGraphNodes;
    std::vector<ed::NodeId> ids(ed::GetSelectedObjectCount());
    ids.resize(ed::GetSelectedNodes(ids.data(), ids.size()));
    for (const auto Node : ids)
    {
        SelectedGraphNodes.emplace(NodeId{ Node });
    }
    return SelectedGraphNodes;
}

// Get all nodes connected to InNode, and if a node is under a Comment node, consider it connected to all nodes under that Comment node.
static std::unordered_set<NodeId> GetLinkedNodes(NodeId InNode)
{
    std::unordered_set<NodeId> VisitedNodes;
    std::unordered_set<NodeId> PendingNodes;
    PendingNodes.emplace(InNode);
    while (PendingNodes.size() > 0)
    {
        NodeId Node = *PendingNodes.begin();
        PendingNodes.erase(Node);
        VisitedNodes.emplace(Node);

        for (const auto link : UEGraphAdapter::CurrentGraph->links)
        {
            PinId pin{ 0 };
            PinId linkedPin{ 0 };
            if (link.from().nodeId() == Node)
            {
                pin = link.from();
                linkedPin = link.to();
            }
            else if (link.to().nodeId() == Node)
            {
                pin = link.to();
                linkedPin = link.from();
            }

            if (linkedPin)
            {
                if (!VisitedNodes.contains(linkedPin.nodeId()))
                {
                    PendingNodes.emplace(linkedPin.nodeId());
                }
            }
        }
    }
    return VisitedNodes;
}

static std::unordered_set<NodeId> SingleNodeSelectionStrategy(NodeId InNode)
{
    return GetLinkedNodes(InNode);
}

static std::unordered_set<NodeId> DoSelectionStrategy(std::unordered_set<NodeId> Selected)
{
    if (Selected.size() != 0)
    {
        return Selected;
    }

    std::unordered_set<NodeId> SelectedGraphNodes;
    for (const auto& Node : UEGraphAdapter::CurrentGraph->nodes)
    {
        SelectedGraphNodes.emplace(Node->id);
    }
    return SelectedGraphNodes;
}

void FFormatter::Format()
{
    if (!PreCommand())
    {
        return;
    }
    auto SelectedNodes = GetSelectedNodes();
    SelectedNodes = DoSelectionStrategy(SelectedNodes);
    for (const auto SelectedNode : SelectedNodes)
    {
        ed::SelectNode(SelectedNode, true);
    }
    auto Graph = UEGraphAdapter::Build(SelectedNodes);
    Graph->Format();
    auto BoundMap = Graph->GetBoundMap();
    delete Graph;
    for (auto [Node, Rect] : BoundMap)
    {
        ed::SetNodePosition(Node, Rect.position);
    }
}

sf::Vector2f projectPointOnLine(sf::Vector2f lineStart, sf::Vector2f lineEnd, sf::Vector2f point)
{
    sf::Vector2f lineDirection = lineEnd - lineStart;
    sf::Vector2f pointVector = point - lineStart;

    const auto  dotProduct = (pointVector.x * lineDirection.x) + (pointVector.y * lineDirection.y);
    const auto  lineMagnitudeSq = (lineDirection.x * lineDirection.x) + (lineDirection.y * lineDirection.y);
    const auto scalarProjection = dotProduct / lineMagnitudeSq;

    // Calculate the projection vector
    sf::Vector2f projection;
    projection.x = lineStart.x + scalarProjection * lineDirection.x;
    projection.y = lineStart.y + scalarProjection * lineDirection.y;

    return projection;
}

void FFormatter::PlaceBlock()
{
    if (!PreCommand())
    {
        return;
    }
    auto SelectedNodes = GetSelectedNodes();
    auto ConnectedNodesLeft = UEGraphAdapter::GetNodesConnected(SelectedNodes, EFormatterPinDirection::In);
    sf::Vector2f ConnectCenter;
    const UFormatterSettings& Settings = UFormatterSettings::Get();
    if (UEGraphAdapter::GetNodesConnectCenter(SelectedNodes, ConnectCenter, EFormatterPinDirection::In))
    {
        auto Center = ConnectCenter;
        auto Direction = sf::Vector2f(1, 0);
        sf::FloatRect Bound = UEGraphAdapter::GetNodesBound(ConnectedNodesLeft);
        auto RightBound = sf::Vector2f(Bound.position.x + Bound.size.x, 0);
        auto LinkedCenter3D = projectPointOnLine(Center, Direction, RightBound);
        auto LinkedCenterTo = sf::Vector2f(LinkedCenter3D) + sf::Vector2f(Settings.HorizontalSpacing, 0);
        UEGraphAdapter::GetNodesConnectCenter(SelectedNodes, ConnectCenter, EFormatterPinDirection::In, true);
        Center = sf::Vector2f(ConnectCenter.x, ConnectCenter.y);
        Direction = sf::Vector2f(-1, 0);
        Bound = UEGraphAdapter::GetNodesBound(SelectedNodes);
        auto LeftBound = sf::Vector2f(Bound.position.x, 0);
        LinkedCenter3D = projectPointOnLine(Center, Direction, LeftBound);
        auto LinkedCenterFrom = sf::Vector2f(LinkedCenter3D);
        sf::Vector2f Offset = LinkedCenterTo - LinkedCenterFrom;
        Translate(SelectedNodes, Offset);
    }
    auto ConnectedNodesRight = UEGraphAdapter::GetNodesConnected(SelectedNodes, EFormatterPinDirection::Out);
    if (UEGraphAdapter::GetNodesConnectCenter(SelectedNodes, ConnectCenter, EFormatterPinDirection::Out))
    {
        auto Center = ConnectCenter;
        auto Direction = sf::Vector2f(-1, 0);
        sf::FloatRect Bound = UEGraphAdapter::GetNodesBound(ConnectedNodesRight);
        auto LeftBound = sf::Vector2f(Bound.position.x, 0);
        auto LinkedCenter3D = projectPointOnLine(Center, Direction, LeftBound);
        auto LinkedCenterTo = sf::Vector2f(LinkedCenter3D) - sf::Vector2f(Settings.HorizontalSpacing, 0);
        UEGraphAdapter::GetNodesConnectCenter(SelectedNodes, ConnectCenter, EFormatterPinDirection::Out, true);
        Center = sf::Vector2f(ConnectCenter.x, ConnectCenter.y);
        Direction = sf::Vector2f(1, 0);
        Bound = UEGraphAdapter::GetNodesBound(SelectedNodes);
        auto RightBound = sf::Vector2f(Bound.position.x + Bound.size.x, 0);
        LinkedCenter3D = projectPointOnLine(Center, Direction, RightBound);
        auto LinkedCenterFrom = sf::Vector2f(LinkedCenter3D);
        sf::Vector2f Offset = LinkedCenterFrom - LinkedCenterTo;
        Translate(ConnectedNodesRight, Offset);
    }
}