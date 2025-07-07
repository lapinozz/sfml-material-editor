/*---------------------------------------------------------------------------------------------
*  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "UEGraphAdapter.h"

#include "Formatter.h"
#include "FormatterSettings.h"

#include "RectHelper.hpp"
#include "guid.hpp"

#include <random>

sf::Vector2f UEGraphAdapter::GetNodeSize(NodeId Node)
{
	const auto size = ed::GetNodeSize(Node);
	return { size.x, size.y };
}

sf::Vector2f UEGraphAdapter::GetNodePosition(NodeId Node)
{
	const auto size = ed::GetNodePosition(Node);
	return { size.x, size.y };
}

sf::FloatRect UEGraphAdapter::GetNodesBound(const std::unordered_set<NodeId> Nodes)
{
	std::optional<sf::FloatRect> Bound;
	for (auto Node : Nodes)
	{
		sf::Vector2f Pos = GetNodePosition(Node);
		sf::Vector2f Size = GetNodeSize(Node);
		sf::FloatRect NodeBound = sf::FloatRect(Pos, Size);
		Bound = Bound ? mergeRects(*Bound, NodeBound) : NodeBound;
	}
	return Bound ? *Bound : sf::FloatRect{};
}


//sf::Vector2f GetPinOffset(PinId Pin) const;

sf::FloatRect GetNodesBound(const std::unordered_set<NodeId> Nodes)
{
	std::optional<sf::FloatRect> Bound;
	for (auto Node : Nodes)
	{
		sf::Vector2f Pos = UEGraphAdapter::GetNodePosition(Node);
		sf::Vector2f Size = UEGraphAdapter::GetNodeSize(Node);
		sf::FloatRect NodeBound = sf::FloatRect(Pos, Size);
		Bound = Bound ? mergeRects(*Bound, NodeBound) : NodeBound;
	}
	return Bound ? *Bound : sf::FloatRect{};
}

FFormatterGraph* UEGraphAdapter::Build(std::unordered_set<NodeId> Nodes)
{
	auto Graph = new FFormatterGraph();
	BuildNodesAndEdges(Graph, Graph->Nodes, Graph->OriginalPinsMap, Nodes);
	auto FoundIsolatedGraphs = FindIsolated(Graph->Nodes);
	delete Graph;

	if (FoundIsolatedGraphs.size() > 1)
	{
		auto DisconnectedGraph = new FDisconnectedGraph();
		for (const auto& IsolatedNodes : FoundIsolatedGraphs)
		{
			auto NewGraph = BuildConnectedGraph(IsolatedNodes);
			DisconnectedGraph->AddGraph(NewGraph);
		}
		return DisconnectedGraph;
	}
	else
	{
		return BuildConnectedGraph(Nodes);
	}
}

/*
FFormatterNode* UEGraphAdapter::CollapseGroup(NodeId MainNode, std::unordered_set<NodeId> Group)
{
	FFormatterNode* Node = new FFormatterNode();
	Node->SetPosition(sf::Vector2f(MainNode->NodePosX, MainNode->NodePosY));
	Node->OriginalNode = MainNode;
	FConnectedGraph* SubGraph = BuildConnectedGraph(Group); // might be fucked
	Node->SetSubGraph(SubGraph);
	SubGraph->SetBorder(0, 0, 0, 0);
	return Node;
}
*/

std::unordered_set<NodeId> UEGraphAdapter::GetDirectConnected(const std::unordered_set<NodeId>& SelectedNodes, EFormatterPinDirection Option)
{
	std::unordered_set<NodeId> DirectConnectedNodes;
	for (auto Node : SelectedNodes)
	{
		for (const auto link : CurrentGraph->links)
		{
			if (link.from().nodeId() == Node)
			{
				auto linked = link.to().nodeId();
				if (Option == EFormatterPinDirection::Out || Option == EFormatterPinDirection::InOut)
				{
					if (!SelectedNodes.contains(linked))
					{
						DirectConnectedNodes.emplace(linked);
					}
				}
			}
			else if (link.to().nodeId() == Node)
			{
				auto linked = link.from().nodeId();
				if (Option == EFormatterPinDirection::In || Option == EFormatterPinDirection::InOut)
				{
					if (!SelectedNodes.contains(linked))
					{
						DirectConnectedNodes.emplace(linked);
					}
				}
			}
		}
	}
	return DirectConnectedNodes;
}

static void GetNodesConnectedRecursively(NodeId RootNode, const std::unordered_set<NodeId>& Excluded, std::unordered_set<NodeId>& OutSet)
{
	std::unordered_set<NodeId> Set;

	for (const auto link : UEGraphAdapter::CurrentGraph->links)
	{
		NodeId linked{0};
		if (link.from().nodeId() == RootNode)
		{
			linked = link.to().nodeId();
		}
		else if (link.to().nodeId() == RootNode)
		{
			linked = link.from().nodeId();
		}

		if (linked)
		{
			if (!Excluded.contains(linked) && !OutSet.contains(linked))
			{
				Set.emplace(linked);
			}
		}
	}

	if (Set.size())
	{
		OutSet.insert(Set.begin(), Set.end());
		for (auto Node : Set)
		{
			GetNodesConnectedRecursively(Node, Excluded, OutSet);
		}
	}
}

std::unordered_set<NodeId> UEGraphAdapter::GetNodesConnected(const std::unordered_set<NodeId>& SelectedNodes, EFormatterPinDirection Option)
{
	std::unordered_set<NodeId> DirectConnectedNodes = GetDirectConnected(SelectedNodes, Option);
	std::unordered_set<NodeId> Result = DirectConnectedNodes;
	for (auto Node : DirectConnectedNodes)
	{
		GetNodesConnectedRecursively(Node, SelectedNodes, Result);
	}
	return Result;
}

bool UEGraphAdapter::GetNodesConnectCenter(const std::unordered_set<NodeId>& SelectedNodes, sf::Vector2f& OutCenter, EFormatterPinDirection Option, bool bInvert)
{
	std::optional<sf::FloatRect> Bound;
	for (auto Node : SelectedNodes)
	{
		for (const auto link : UEGraphAdapter::CurrentGraph->links)
		{
			PinId pin{ 0 };
			PinId linkedPin{ 0 };
			NodeId linked{ 0 };
			if (link.from().nodeId() == Node)
			{
				pin = link.from();
				linkedPin = link.to();
				linked = linkedPin.nodeId();
			}
			else if (link.to().nodeId() == Node)
			{
				pin = link.to();
				linkedPin = link.from();
				linked = linkedPin.nodeId();
			}

			if (pin)
			{
				if ((Option == EFormatterPinDirection::In && pin.direction() == PinDirection::In) ||
					(Option == EFormatterPinDirection::Out && pin.direction() == PinDirection::Out) ||
					(Option == EFormatterPinDirection::InOut))
				{
					if (!SelectedNodes.contains(linked))
					{
						auto Pos = GetNodePosition(bInvert ? Node : linked);
						auto Size = GetNodeSize(bInvert ? Node : linked);
						sf::FloatRect NodeBoundRect = sf::FloatRect(Pos, Size);
						Bound = Bound ? mergeRects(*Bound, NodeBoundRect) : NodeBoundRect;
					}
				}
			}
		}
	}
	if (Bound)
	{
		OutCenter = Bound->position + Bound->size / 2;
		return true;
	}
	else
	{
		return false;
	}
}

void UEGraphAdapter::BuildNodes(FFormatterGraph* Graph, std::unordered_set<NodeId> SelectedNodes)
{
	const UFormatterSettings& Settings = UFormatterSettings::Get();

	for (auto Node : SelectedNodes)
	{
		FFormatterNode* NodeData = FormatterNodeFromUEGraphNode(Node);
		Graph->AddNode(NodeData);
	}
}

void UEGraphAdapter::BuildEdges(std::vector<FFormatterNode*>& Nodes, std::unordered_map<PinId, FFormatterPin*>& PinsMap, std::unordered_set<NodeId> SelectedNodes)
{
	for (auto Node : Nodes)
	{
		auto Edges = GetEdgeForNode(Node, PinsMap, SelectedNodes);
		for (auto Edge : Edges)
		{
			Node->Connect(Edge.From, Edge.To, Edge.Weight);
		}
	}
}

void UEGraphAdapter::BuildNodesAndEdges(FFormatterGraph* Graph, std::vector<FFormatterNode*>& Nodes, std::unordered_map<PinId, FFormatterPin*>& PinsMap, const std::unordered_set<NodeId>& SelectedNodes)
{
	BuildNodes(Graph, SelectedNodes);
	BuildEdges(Nodes, PinsMap, SelectedNodes);
	std::ranges::sort(Nodes, [Graph](const FFormatterNode* A, const FFormatterNode* B)
	{
		return A->GetPosition().y < B->GetPosition().y;
	});
}

static float GetEdgeWeight(auto StartPin)
{
	return 99;
}

std::vector<FFormatterEdge> UEGraphAdapter::GetEdgeForNode(FFormatterNode* Node, std::unordered_map<PinId, FFormatterPin*>& PinsMap, std::unordered_set<NodeId> SelectedNodes)
{
	std::vector<FFormatterEdge> Result;
	if (Node->SubGraph)
	{
		const std::unordered_set<NodeId> InnerSelectedNodes = Node->SubGraph->GetOriginalNodes();
		for (NodeId SelectedNode : InnerSelectedNodes)
		{
			for (const auto link : UEGraphAdapter::CurrentGraph->links)
			{
				PinId pin{ 0 };
				PinId linkedPin{ 0 };
				if (link.from().nodeId() == SelectedNode)
				{
					pin = link.from();
					linkedPin = link.to();
				}
				else if (link.to().nodeId() == SelectedNode)
				{
					pin = link.to();
					linkedPin = link.from();
				}

				if (linkedPin)
				{
					const auto LinkedToNode = linkedPin.nodeId();
					if (InnerSelectedNodes.contains(LinkedToNode) || !SelectedNodes.contains(LinkedToNode))
					{
						continue;
					}
					FFormatterPin* From = PinsMap[pin];
					FFormatterPin* To = PinsMap[linkedPin];
					Result.push_back(FFormatterEdge{ From, To, GetEdgeWeight(pin) });
				}
			}
		}
	}
	else
	{
		NodeId OriginalNode = Node->OriginalNode;

		for (const auto link : UEGraphAdapter::CurrentGraph->links)
		{
			PinId pin{ 0 };
			PinId linkedPin{ 0 };
			if (link.from().nodeId() == OriginalNode)
			{
				pin = link.from();
				linkedPin = link.to();
			}
			else if (link.to().nodeId() == OriginalNode)
			{
				pin = link.to();
				linkedPin = link.from();
			}

			if (linkedPin)
			{
				const auto LinkedToNode = linkedPin.nodeId();
				if (!SelectedNodes.contains(LinkedToNode))
				{
					continue;
				}
				FFormatterPin* From = PinsMap[pin];
				FFormatterPin* To = PinsMap[linkedPin];
				Result.push_back(FFormatterEdge{ From, To, GetEdgeWeight(pin) });
			}
		}
	}
	return Result;
}

std::vector<std::unordered_set<NodeId>> UEGraphAdapter::FindIsolated(const std::vector<FFormatterNode*>& Nodes)
{
	std::vector<std::unordered_set<NodeId>> Result;
	std::unordered_set<FFormatterNode*> CheckedNodes;
	std::vector<FFormatterNode*> Stack;
	for (auto Node : Nodes)
	{
		if (!CheckedNodes.contains(Node))
		{
			CheckedNodes.emplace(Node);
			Stack.push_back(Node);
		}
		std::unordered_set<NodeId> IsolatedNodes;
		while (Stack.size() != 0)
		{
			FFormatterNode* Top = Stack.back();
			Stack.erase(Stack.end() - 1);

			IsolatedNodes.emplace(Top->OriginalNode);
			if (Top->SubGraph != nullptr)
			{
				for (auto OriginalNode : Top->SubGraph->GetOriginalNodes())
				{
					IsolatedNodes.emplace(OriginalNode);
				}
			}
			std::vector<FFormatterNode*> ConnectedNodes = Top->GetSuccessors();
			std::vector<FFormatterNode*> Predecessors = Top->GetPredecessors();
			ConnectedNodes.insert(ConnectedNodes.end(), Predecessors.begin(), Predecessors.end());
			for (auto ConnectedNode : ConnectedNodes)
			{
				if (!CheckedNodes.contains(ConnectedNode))
				{
					Stack.push_back(ConnectedNode);
					CheckedNodes.emplace(ConnectedNode);
				}
			}
		}
		if (IsolatedNodes.size() != 0)
		{
			Result.push_back(IsolatedNodes);
		}
	}
	return Result;
}

FFormatterNode* UEGraphAdapter::FormatterNodeFromUEGraphNode(NodeId InNode)
{
	FFormatterNode* Node = new FFormatterNode();
	Node->Guid = {InNode.Get()};
	Node->OriginalNode = InNode;
	Node->SubGraph = nullptr;
	Node->Size = GetNodeSize(InNode);
	Node->PathDepth = 0;
	Node->SetPosition(GetNodePosition(InNode));

	for (const auto link : UEGraphAdapter::CurrentGraph->links)
	{
		PinId pin{ 0 };
		if (link.from().nodeId() == InNode)
		{
			pin = link.from();
		}
		else if (link.to().nodeId() == InNode)
		{
			pin = link.to();
		}

		if (pin)
		{
			auto NewPin = new FFormatterPin;
			NewPin->Guid = NewGuid();
			NewPin->OriginalPin = pin;
			NewPin->Direction = pin.direction() == PinDirection::In ? EFormatterPinDirection::In : EFormatterPinDirection::Out;
			NewPin->OwningNode = Node;
			NewPin->NodeOffset = Node->Size / 2.f; // FFormatter::Instance().GetPinOffset(Pin);
			if (pin.direction() == PinDirection::In)
			{
				Node->InPins.push_back(NewPin);
			}
			else
			{
				Node->OutPins.push_back(NewPin);
			}
		}
	}

	return Node;
}

FConnectedGraph* UEGraphAdapter::BuildConnectedGraph(const std::unordered_set<NodeId>& SelectedNodes)
{
	FConnectedGraph* Graph = new FConnectedGraph();
	BuildNodesAndEdges(Graph, Graph->Nodes, Graph->OriginalPinsMap, SelectedNodes);
	return Graph;
}