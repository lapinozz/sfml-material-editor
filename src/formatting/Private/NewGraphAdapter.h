/*---------------------------------------------------------------------------------------------
*  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once
#include "FormatterGraph.h"
#include "formatting/graph_layout/graph_layout.h"

class NewGraphAdapter
{
public:
    static std::vector<std::vector<FFormatterNode*>> GetLayeredListFromNewGraph(const FConnectedGraph* Graph);
    static graph_layout::graph_t* BuildGraph(std::unordered_set<UEdGraphNode*> Nodes);
    static void BuildNodes(graph_layout::graph_t* Graph, std::unordered_set<UEdGraphNode*> Nodes);
    static void BuildEdges(graph_layout::graph_t* Graph, std::unordered_set<UEdGraphNode*> SelectedNodes);
    static void AddNode(graph_layout::graph_t* Graph, UEdGraphNode* Node, graph_layout::graph_t* SubGraph);
    static graph_layout::graph_t* CollapseCommentNode(UEdGraphNode* CommentNode, std::unordered_set<UEdGraphNode*> NodesUnderComment);
    static graph_layout::graph_t* CollapseGroup(UEdGraphNode* MainNode, std::unordered_set<UEdGraphNode*> Group);
    static void BuildEdgeForNode(graph_layout::graph_t* Graph, graph_layout::node_t* Node, std::unordered_set<UEdGraphNode*> SelectedNodes);
    static std::unordered_map<UEdGraphNode*, sf::FloatRect> GetBoundMap(graph_layout::graph_t* Graph);
};
