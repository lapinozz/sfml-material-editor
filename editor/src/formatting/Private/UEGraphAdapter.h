/*---------------------------------------------------------------------------------------------
*  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once

#include "FormatterGraph.h"

#include "graph.hpp"

class UEGraphAdapter
{
public:
    static FFormatterGraph* Build(std::unordered_set<NodeId> Nodes);
    //static FFormatterNode* CollapseGroup(NodeId MainNode, std::unordered_set<NodeId> Group);
    static std::unordered_set<NodeId> GetDirectConnected(const std::unordered_set<NodeId>& SelectedNodes, EFormatterPinDirection Option);
    static std::unordered_set<NodeId> GetNodesConnected(const std::unordered_set<NodeId>& SelectedNodes, EFormatterPinDirection Option);
    static bool GetNodesConnectCenter(const std::unordered_set<NodeId>& SelectedNodes, sf::Vector2f& OutCenter, EFormatterPinDirection Option, bool bInvert = false);
    static void BuildNodes(FFormatterGraph* Graph, std::unordered_set<NodeId> SelectedNodes);
    static void BuildEdges(std::vector<FFormatterNode*>& Nodes, std::unordered_map<PinId, FFormatterPin*>& PinsMap, std::unordered_set<NodeId> SelectedNodes);
    static void BuildNodesAndEdges(FFormatterGraph* Graph, std::vector<FFormatterNode*>& Nodes, std::unordered_map<PinId, FFormatterPin*>& PinsMap, const std::unordered_set<NodeId>& SelectedNodes);
    static std::vector<FFormatterEdge> GetEdgeForNode(FFormatterNode* Node, std::unordered_map<PinId, FFormatterPin*>& PinsMap, std::unordered_set<NodeId> SelectedNodes);
    static std::vector<std::unordered_set<NodeId>> FindIsolated(const std::vector<FFormatterNode*>& Nodes);


    static FFormatterNode* FormatterNodeFromUEGraphNode(NodeId InNode);
    static FConnectedGraph* BuildConnectedGraph(const std::unordered_set<NodeId>& SelectedNodes);

    static inline Graph* CurrentGraph = nullptr;

    static sf::Vector2f GetNodeSize(NodeId Node);
    static sf::Vector2f GetNodePosition(NodeId Node);
    static sf::FloatRect GetNodesBound(const std::unordered_set<NodeId> Nodes);
};
