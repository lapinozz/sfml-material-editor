/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "FormatterGraph.h"

#include "EvenlyPlaceStrategy.h"
#include "FastAndSimplePositioningStrategy.h"
#include "PriorityPositioningStrategy.h"

#include <random>
#include <algorithm>

#include "RectHelper.hpp"
#include "guid.hpp"

bool FFormatterEdge::IsCrossing(const FFormatterEdge* Edge) const
{
    return (From->IndexInLayer < Edge->From->IndexInLayer && To->IndexInLayer > Edge->To->IndexInLayer) || (From->IndexInLayer > Edge->From->IndexInLayer && To->IndexInLayer < Edge->To->IndexInLayer);
}

bool FFormatterEdge::IsInnerSegment() const
{
    return From->OwningNode->OriginalNode && To->OwningNode->OriginalNode;
}

FFormatterNode::FFormatterNode(const FFormatterNode& Other)
    : Guid(Other.Guid)
    , OriginalNode(Other.OriginalNode)
    , Size(Other.Size)
    , PathDepth(Other.PathDepth)
    , Position(Other.Position)
{
    if (Other.SubGraph != nullptr)
    {
        SubGraph = Other.SubGraph->Clone();
    }
    else
    {
        SubGraph = nullptr;
    }
    for (auto Pin : Other.InPins)
    {
        auto NewPin = new FFormatterPin;
        NewPin->Guid = Pin->Guid;
        NewPin->OriginalPin = Pin->OriginalPin;
        NewPin->Direction = Pin->Direction;
        NewPin->OwningNode = this;
        NewPin->NodeOffset = Pin->NodeOffset;
        InPins.push_back(NewPin);
    }
    for (auto Pin : Other.OutPins)
    {
        auto NewPin = new FFormatterPin;
        NewPin->Guid = Pin->Guid;
        NewPin->OriginalPin = Pin->OriginalPin;
        NewPin->Direction = Pin->Direction;
        NewPin->OwningNode = this;
        NewPin->NodeOffset = Pin->NodeOffset;
        OutPins.push_back(NewPin);
    }
}

FFormatterNode::FFormatterNode()
    : Guid(NewGuid())
    , Size(sf::Vector2f(1, 1))
    , PathDepth(0)
    , PositioningPriority(INT_MAX)
    , Position({})
{
}

FFormatterNode* FFormatterNode::CreateDummy()
{
    auto DummyNode = new FFormatterNode();
    auto InPin = new FFormatterPin;
    InPin->Guid = NewGuid();
    InPin->OriginalPin = { 0 };
    InPin->Direction = EFormatterPinDirection::In;
    InPin->OwningNode = DummyNode;
    InPin->NodeOffset = {};

    auto OutPin = new FFormatterPin;
    OutPin->Guid = NewGuid();
    OutPin->OriginalPin = { 0 };
    OutPin->Direction = EFormatterPinDirection::Out;
    OutPin->OwningNode = DummyNode;
    OutPin->NodeOffset = {};

    DummyNode->InPins.push_back(InPin);
    DummyNode->OutPins.push_back(OutPin);
    return DummyNode;
}

void FFormatterNode::Connect(FFormatterPin* SourcePin, FFormatterPin* TargetPin, float Weight)
{
    const auto Edge = new FFormatterEdge;
    Edge->From = SourcePin;
    Edge->To = TargetPin;
    Edge->Weight = Weight;
    if (SourcePin->Direction == EFormatterPinDirection::Out)
    {
        OutEdges.push_back(Edge);
    }
    else
    {
        InEdges.push_back(Edge);
    }
}

void FFormatterNode::Disconnect(FFormatterPin* SourcePin, FFormatterPin* TargetPin)
{
    const auto Predicate = [SourcePin, TargetPin](const FFormatterEdge* Edge)
    {
        return Edge->From == SourcePin && Edge->To == TargetPin;
    };
    if (SourcePin->Direction == EFormatterPinDirection::Out)
    {
        const auto it = std::ranges::find_if(OutEdges, Predicate);
        if (it != OutEdges.end())
        {
            delete *it;
            OutEdges.erase(it);
        }
    }
    else
    {
        const auto it = std::ranges::find_if(InEdges, Predicate);
        if (it != InEdges.end())
        {
            delete* it;
            InEdges.erase(it);
        }
    }
}

void FFormatterNode::AddPin(FFormatterPin* Pin)
{
    if (Pin->Direction == EFormatterPinDirection::In)
    {
        InPins.push_back(Pin);
    }
    else
    {
        OutPins.push_back(Pin);
    }
}

std::vector<FFormatterNode*> FFormatterNode::GetSuccessors() const
{
    std::vector<FFormatterNode*> Result;
    for (auto Edge : OutEdges)
    {
        Result.push_back(Edge->To->OwningNode);
    }
    return Result;
}

std::vector<FFormatterNode*> FFormatterNode::GetPredecessors() const
{
    std::vector<FFormatterNode*> Result;
    for (auto Edge : InEdges)
    {
        Result.push_back(Edge->To->OwningNode);
    }
    return Result;
}

bool FFormatterNode::IsSource() const
{
    return InEdges.size() == 0;
}

bool FFormatterNode::IsSink() const
{
    return OutEdges.size() == 0;
}

bool FFormatterNode::AnySuccessorPathDepthEqu0() const
{
    for (auto OutEdge : OutEdges)
    {
        if (OutEdge->To->OwningNode->PathDepth == 0)
        {
            return true;
        }
    }
    return false;
}

float FFormatterNode::GetLinkedPositionToNode(const FFormatterNode* Node, EFormatterPinDirection Direction, bool IsHorizontalDirection)
{
    auto& Edges = Direction == EFormatterPinDirection::In ? InEdges : OutEdges;
    float MedianPosition = 0.0f;
    std::int32_t Count = 0;
    for (auto Edge : Edges)
    {
        if (Edge->To->OwningNode == Node)
        {
            if (IsHorizontalDirection)
            {
                MedianPosition += Edge->From->NodeOffset.y;
            }
            else
            {
                MedianPosition += Edge->From->NodeOffset.x;
            }
            ++Count;
        }
    }
    if (Count == 0)
    {
        return 0.0f;
    }
    return MedianPosition / Count;
}

float FFormatterNode::GetMaxWeight(EFormatterPinDirection Direction)
{
    auto& Edges = Direction == EFormatterPinDirection::In ? InEdges : OutEdges;
    float MaxWeight = 0.0f;
    for (auto Edge : Edges)
    {
        if (MaxWeight < Edge->Weight)
        {
            MaxWeight = Edge->Weight;
        }
    }
    return MaxWeight;
}

float FFormatterNode::GetMaxWeightToNode(const FFormatterNode* Node, EFormatterPinDirection Direction)
{
    auto& Edges = Direction == EFormatterPinDirection::In ? InEdges : OutEdges;
    float MaxWeight = 0.0f;
    for (auto Edge : Edges)
    {
        if (Edge->To->OwningNode == Node && MaxWeight < Edge->Weight)
        {
            MaxWeight = Edge->Weight;
        }
    }
    return MaxWeight;
}

bool FFormatterNode::IsCrossingInnerSegment(const std::vector<FFormatterNode*>& LowerLayer, const std::vector<FFormatterNode*>& UpperLayer) const
{
    auto EdgesLinkedToUpper = GetEdgeLinkedToLayer(UpperLayer, EFormatterPinDirection::In);
    auto EdgesBetweenTwoLayers = GetEdgeBetweenTwoLayer(LowerLayer, UpperLayer, this);
    for (auto EdgeLinkedToUpper : EdgesLinkedToUpper)
    {
        for (auto EdgeBetweenTwoLayers : EdgesBetweenTwoLayers)
        {
            if (EdgeBetweenTwoLayers->IsInnerSegment() && EdgeLinkedToUpper->IsCrossing(EdgeBetweenTwoLayers))
            {
                return true;
            }
        }
    }
    return false;
}

FFormatterNode* FFormatterNode::GetMedianUpper() const
{
    std::vector<FFormatterNode*> UpperNodes;
    for (auto InEdge : InEdges)
    {
        if (std::ranges::find(UpperNodes, InEdge->To->OwningNode) == UpperNodes.end())
        {
            UpperNodes.push_back(InEdge->To->OwningNode);
        }
    }
    if (UpperNodes.size() > 0)
    {
        const std::int32_t m = UpperNodes.size() / 2;
        return UpperNodes[m];
    }
    return nullptr;
}

FFormatterNode* FFormatterNode::GetMedianLower() const
{
    std::vector<FFormatterNode*> LowerNodes;
    for (auto OutEdge : OutEdges)
    {
        if (std::ranges::find(LowerNodes, OutEdge->To->OwningNode) == LowerNodes.end())
        {
            LowerNodes.push_back(OutEdge->To->OwningNode);
        }
    }
    if (LowerNodes.size() > 0)
    {
        const std::int32_t m = LowerNodes.size() / 2;
        return LowerNodes[m];
    }
    return nullptr;
}

std::vector<FFormatterNode*> FFormatterNode::GetUppers() const
{
    std::unordered_set<FFormatterNode*> UpperNodes;
    for (auto InEdge : InEdges)
    {
        UpperNodes.emplace(InEdge->To->OwningNode);
    }
    return std::vector<FFormatterNode*>(UpperNodes.begin(), UpperNodes.end());
}

std::vector<FFormatterNode*> FFormatterNode::GetLowers() const
{
    std::unordered_set<FFormatterNode*> LowerNodes;
    for (auto OutEdge : OutEdges)
    {
        LowerNodes.emplace(OutEdge->To->OwningNode);
    }
    return std::vector<FFormatterNode*>(LowerNodes.begin(), LowerNodes.end());
}

std::int32_t FFormatterNode::GetInputPinCount() const
{
    return InPins.size();
}

std::int32_t FFormatterNode::GetInputPinIndex(FFormatterPin* InputPin) const
{
    const auto it = std::ranges::find(InPins, InputPin);
    assert(it != InPins.end());

    return std::ranges::distance(InPins.begin(), it);
}

std::int32_t FFormatterNode::GetOutputPinCount() const
{
    return OutPins.size();
}

std::int32_t FFormatterNode::GetOutputPinIndex(FFormatterPin* OutputPin) const
{
    const auto it = std::ranges::find(OutPins, OutputPin);
    assert(it != OutPins.end());

    return std::ranges::distance(OutPins.begin(), it);
}

std::vector<FFormatterEdge*> FFormatterNode::GetEdgeLinkedToLayer(const std::vector<FFormatterNode*>& Layer, EFormatterPinDirection Direction) const
{
    std::vector<FFormatterEdge*> Result;
    const std::vector<FFormatterEdge*>& Edges = Direction == EFormatterPinDirection::Out ? OutEdges : InEdges;
    for (auto Edge : Edges)
    {
        for (auto NextLayerNode : Layer)
        {
            if (Edge->To->OwningNode == NextLayerNode)
            {
                Result.push_back(Edge);
            }
        }
    }
    return Result;
}

float FFormatterNode::CalcBarycenter(const std::vector<FFormatterNode*>& Layer, EFormatterPinDirection Direction) const
{
    auto Edges = GetEdgeLinkedToLayer(Layer, Direction);
    if (Edges.size() == 0)
    {
        return 0.0f;
    }
    float Sum = 0.0f;
    for (auto Edge : Edges)
    {
        Sum += Edge->To->IndexInLayer;
    }
    return Sum / Edges.size();
}

std::int32_t FFormatterNode::CalcPriority(EFormatterPinDirection Direction) const
{
    if (!OriginalNode)
    {
        return 0;
    }
    return Direction == EFormatterPinDirection::Out ? OutEdges.size() : InEdges.size();
}

FFormatterNode::~FFormatterNode()
{
    for (auto Edge : InEdges)
    {
        delete Edge;
    }
    for (auto Edge : OutEdges)
    {
        delete Edge;
    }
    for (auto Pin : InPins)
    {
        delete Pin;
    }
    for (auto Pin : OutPins)
    {
        delete Pin;
    }
    delete SubGraph;
}

void FFormatterNode::InitPosition(sf::Vector2f InPosition)
{
    Position = InPosition;
}

void FFormatterNode::SetPosition(sf::Vector2f InPosition)
{
    const sf::Vector2f Offset = InPosition - Position;
    Position = InPosition;
    if (SubGraph != nullptr)
    {
        SubGraph->OffsetBy(Offset);
    }
}

sf::Vector2f FFormatterNode::GetPosition() const
{
    return Position;
}

void FFormatterNode::SetSubGraph(FFormatterGraph* InSubGraph)
{
    SubGraph = InSubGraph;
    auto SubGraphInPins = SubGraph->GetInputPins();
    auto SubGraphOutPins = SubGraph->GetOutputPins();
    for (auto Pin : SubGraphInPins)
    {
        auto NewPin = new FFormatterPin();
        NewPin->Guid = Pin->Guid;
        NewPin->OwningNode = this;
        NewPin->Direction = Pin->Direction;
        NewPin->NodeOffset = Pin->NodeOffset;
        NewPin->OriginalPin = Pin->OriginalPin;
        InPins.push_back(NewPin);
    }
    for (auto Pin : SubGraphOutPins)
    {
        auto NewPin = new FFormatterPin();
        NewPin->Guid = Pin->Guid;
        NewPin->OwningNode = this;
        NewPin->Direction = Pin->Direction;
        NewPin->NodeOffset = Pin->NodeOffset;
        NewPin->OriginalPin = Pin->OriginalPin;
        OutPins.push_back(NewPin);
    }
}

void FFormatterNode::UpdatePinsOffset(sf::Vector2f Border)
{
    if (SubGraph != nullptr)
    {
        auto PinsOffset = SubGraph->GetPinsOffset();
        for (auto Pin : InPins)
        {
            if (PinsOffset.contains(Pin->OriginalPin))
            {
                Pin->NodeOffset = PinsOffset[Pin->OriginalPin] + Border;
            }
        }
        for (auto Pin : OutPins)
        {
            if (PinsOffset.contains(Pin->OriginalPin))
            {
                Pin->NodeOffset = PinsOffset[Pin->OriginalPin] + Border;
            }
        }
        std::ranges::sort(InPins, [](const FFormatterPin* A, const FFormatterPin* B)
                    { return A->NodeOffset.y < B->NodeOffset.y; });
        std::ranges::sort(OutPins, [](const FFormatterPin* A, const FFormatterPin* B)
                     { return A->NodeOffset.y < B->NodeOffset.y; });
    }
}

std::vector<FFormatterNode*> FFormatterNode::GetSuccessorsForNodes(std::unordered_set<FFormatterNode*> Nodes)
{
    std::vector<FFormatterNode*> Result;
    for (auto Node : Nodes)
    {
        for (auto OutEdge : Node->OutEdges)
        {
            if (!Nodes.contains(OutEdge->To->OwningNode))
            {
                Result.push_back(OutEdge->To->OwningNode);
            }
        }
    }
    return Result;
}

void FFormatterNode::CalculatePinsIndexInLayer(const std::vector<FFormatterNode*>& Layer)
{
    std::int32_t InPinStartIndex = 0, OutPinStartIndex = 0;
    for (std::int32_t j = 0; j < Layer.size(); j++)
    {
        for (auto InPin : Layer[j]->InPins)
        {
            InPin->IndexInLayer = InPinStartIndex + Layer[j]->GetInputPinIndex(InPin);
        }
        for (auto OutPin : Layer[j]->OutPins)
        {
            OutPin->IndexInLayer = OutPinStartIndex + Layer[j]->GetOutputPinIndex(OutPin);
        }
        OutPinStartIndex += Layer[j]->GetOutputPinCount();
        InPinStartIndex += Layer[j]->GetInputPinCount();
    }
}

void FFormatterNode::CalculatePinsIndex(const std::vector<std::vector<FFormatterNode*>>& Order)
{
    for (std::int32_t i = 0; i < Order.size(); i++)
    {
        auto& Layer = Order[i];
        CalculatePinsIndexInLayer(Layer);
    }
}

std::vector<FFormatterNode*> FConnectedGraph::GetNodesGreaterThan(std::int32_t i, std::unordered_set<FFormatterNode*>& Excluded)
{
    std::vector<FFormatterNode*> Result;
    for (auto Node : Nodes)
    {
        if (!Excluded.contains(Node) && Node->PathDepth >= i)
        {
            Result.push_back(Node);
        }
    }
    return Result;
}

void FConnectedGraph::RemoveNode(FFormatterNode* NodeToRemove)
{
    std::vector<FFormatterEdge*> Edges = NodeToRemove->InEdges;
    for (auto Edge : Edges)
    {
        Edge->To->OwningNode->Disconnect(Edge->To, Edge->From);
    }
    Edges = NodeToRemove->OutEdges;
    for (auto Edge : Edges)
    {
        Edge->To->OwningNode->Disconnect(Edge->To, Edge->From);
    }
    std::erase(Nodes, NodeToRemove);
    NodesMap.erase(NodeToRemove->Guid);
    SubGraphs.erase(NodeToRemove->Guid);
    for (auto Pin : NodeToRemove->InPins)
    {
        OriginalPinsMap.erase(Pin->OriginalPin);
        PinsMap.erase(Pin->Guid);
    }
    for (auto Pin : NodeToRemove->OutPins)
    {
        OriginalPinsMap.erase(Pin->OriginalPin);
        PinsMap.erase(Pin->Guid);
    }
    delete NodeToRemove;
}

void FConnectedGraph::RemoveCycle()
{
    auto ClonedGraph = new FConnectedGraph(*this);
    while (auto SourceNode = ClonedGraph->FindSourceNode())
    {
        ClonedGraph->RemoveNode(SourceNode);
    }
    while (auto SinkNode = ClonedGraph->FindSinkNode())
    {
        ClonedGraph->RemoveNode(SinkNode);
    }
    while (auto MedianNode = ClonedGraph->FindMaxInOutWeightDiffNode())
    {
        for (auto Edge : MedianNode->InEdges)
        {
            FFormatterPin* From = PinsMap[Edge->From->Guid];
            FFormatterPin* To = PinsMap[Edge->To->Guid];
            NodesMap[MedianNode->Guid]->Disconnect(From, To);
            To->OwningNode->Disconnect(To, From);
        }
        ClonedGraph->RemoveNode(MedianNode);
    }
    delete ClonedGraph;
}

FFormatterNode* FConnectedGraph::FindSourceNode() const
{
    for (auto Node : Nodes)
    {
        if (Node->IsSource())
        {
            return Node;
        }
    }
    return nullptr;
}

FFormatterNode* FConnectedGraph::FindSinkNode() const
{
    for (auto Node : Nodes)
    {
        if (Node->IsSink())
        {
            return Node;
        }
    }
    return nullptr;
}

FFormatterNode* FConnectedGraph::FindMaxInOutWeightDiffNode() const
{
    auto EdgesWeightSum = [](const std::vector<FFormatterEdge*>& Edges)
    {
        float Sum = 0;
        for (const auto Edge : Edges)
        {
            Sum += Edge->Weight;
        }
        return Sum;
    };
    FFormatterNode* Result = nullptr;
    std::int32_t MaxWeight = -INT32_MAX;
    for (const auto Node : Nodes)
    {
        const std::int32_t Diff = EdgesWeightSum(Node->OutEdges) - EdgesWeightSum(Node->InEdges);
        if (Diff > MaxWeight)
        {
            MaxWeight = Diff;
            Result = Node;
        }
    }
    return Result;
}

std::vector<sf::FloatRect> FFormatterGraph::CalculateLayersBound(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes, bool IsHorizontalDirection)
{
    std::vector<sf::FloatRect> LayersBound;
    std::optional<sf::FloatRect> TotalBound;
    sf::Vector2f Spacing;
    if (IsHorizontalDirection)
    {
        Spacing = sf::Vector2f(HorizontalSpacing, 0);
    }
    else
    {
        Spacing = sf::Vector2f(0, VerticalSpacing);
    }

    for (std::int32_t i = 0; i < InLayeredNodes.size(); i++)
    {
        const auto& Layer = InLayeredNodes[i];
        std::optional<sf::FloatRect> Bound;
        sf::Vector2f Position;
        if (TotalBound)
        {
            Position = TotalBound->position + TotalBound->size + Spacing;
        }
        else
        {
            Position = sf::Vector2f(0, 0);
        }
        for (auto Node : Layer)
        {
            if (Bound)
            {
                Bound = mergeRects(*Bound, sf::FloatRect(Position, Node->Size));
            }
            else
            {
                Bound = sf::FloatRect(Position, Node->Size);
            }
        }
        LayersBound.push_back(*Bound);
        if (TotalBound)
        {
            TotalBound = mergeRects(*TotalBound, *Bound);
        }
        else
        {
            TotalBound = Bound;
        }
    }
    return LayersBound;
}

FFormatterGraph::FFormatterGraph()
{
}

void FFormatterGraph::AddNode(FFormatterNode* InNode)
{
    Nodes.push_back(InNode);
    NodesMap.emplace(InNode->Guid, InNode);
    if (InNode->SubGraph != nullptr)
    {
        SubGraphs.emplace(InNode->Guid, InNode->SubGraph);
    }
    for (auto Pin : InNode->InPins)
    {
        if (Pin->OriginalPin)
        {
            OriginalPinsMap.emplace(Pin->OriginalPin, Pin);
        }
        PinsMap.emplace(Pin->Guid, Pin);
    }
    for (auto Pin : InNode->OutPins)
    {
        if (Pin->OriginalPin)
        {
            OriginalPinsMap.emplace(Pin->OriginalPin, Pin);
        }
        PinsMap.emplace(Pin->Guid, Pin);
    }
}

void FFormatterGraph::SetPosition(const sf::Vector2f& Position)
{
    const sf::Vector2f Offset = Position - TotalBound->position;
    OffsetBy(Offset);
}

void FFormatterGraph::SetBorder(float Left, float Top, float Right, float Bottom)
{
    this->Border = sf::FloatRect(sf::Vector2f(Left, Top), sf::Vector2f(Right, Bottom) - sf::Vector2f(Left, Top));
}

sf::FloatRect FFormatterGraph::GetBorder() const
{
    return *Border;
}

FFormatterGraph::FFormatterGraph(const FFormatterGraph& Other)
{
    for (auto Node : Other.Nodes)
    {
        FFormatterNode* Cloned = new FFormatterNode(*Node);
        AddNode(Cloned);
    }
    for (auto Node : Other.Nodes)
    {
        for (auto Edge : Node->InEdges)
        {
            FFormatterPin* From = PinsMap[Edge->From->Guid];
            FFormatterPin* To = PinsMap[Edge->To->Guid];
            NodesMap[Node->Guid]->Connect(From, To, Edge->Weight);
        }
        for (auto Edge : Node->OutEdges)
        {
            FFormatterPin* From = PinsMap[Edge->From->Guid];
            FFormatterPin* To = PinsMap[Edge->To->Guid];
            NodesMap[Node->Guid]->Connect(From, To, Edge->Weight);
        }
    }

    TotalBound = Other.TotalBound;
    Border = Other.Border;
}

FFormatterGraph::~FFormatterGraph()
{
    if (IsNodeDetached)
    {
        return;
    }
 
    for (auto Node : Nodes)
    {
        delete Node;
    }
}

void FFormatterGraph::DetachAndDestroy()
{
    IsNodeDetached = true;
    delete this;
}

FFormatterGraph* FFormatterGraph::Clone()
{
    return nullptr;
}

void FDisconnectedGraph::AddGraph(FFormatterGraph* Graph)
{
    ConnectedGraphs.push_back(Graph);
}

FDisconnectedGraph::~FDisconnectedGraph()
{
    for (auto Graph : ConnectedGraphs)
    {
        delete Graph;
    }
}

std::unordered_map<PinId, sf::Vector2f> FDisconnectedGraph::GetPinsOffset()
{
    std::unordered_map<PinId, sf::Vector2f> Result;
    for (auto Graph : ConnectedGraphs)
    {
        auto SubBound = Graph->GetTotalBound();
        auto Offset = SubBound.position - GetTotalBound().position;
        auto SubOffsets = Graph->GetPinsOffset();
        for (auto& SubOffsetPair : SubOffsets)
        {
            SubOffsetPair.second = SubOffsetPair.second + Offset;
        }

        for (const auto& pair : SubOffsets)
        {
            Result[pair.first] = pair.second;
        };
    }
    return Result;
}

std::vector<FFormatterPin*> FDisconnectedGraph::GetInputPins() const
{
    std::unordered_set<FFormatterPin*> Result;
    for (auto Graph : ConnectedGraphs)
    {
        auto pins = Graph->GetInputPins();
        Result.insert(pins.begin(), pins.end());
    }
    return std::vector<FFormatterPin*>(Result.begin(), Result.end());
}

std::vector<FFormatterPin*> FDisconnectedGraph::GetOutputPins() const
{
    std::unordered_set<FFormatterPin*> Result;
    for (auto Graph : ConnectedGraphs)
    {
        auto pins = Graph->GetOutputPins();
        Result.insert(pins.begin(), pins.end());
    }
    return std::vector<FFormatterPin*>(Result.begin(), Result.end());
}

std::unordered_set<NodeId> FDisconnectedGraph::GetOriginalNodes() const
{
    std::unordered_set<NodeId> Result;
    for (auto Graph : ConnectedGraphs)
    {
        auto nodes = Graph->GetOriginalNodes();
        Result.insert(nodes.begin(), nodes.end());
    }
    return Result;
}

void FDisconnectedGraph::Format()
{
    std::optional<sf::FloatRect> PreBound;
    for (auto Graph : ConnectedGraphs)
    {
        Graph->Format();

        if (PreBound)
        {
            const auto min = PreBound->position;
            const auto max = PreBound->position + PreBound->size;
            sf::Vector2f TopRight = sf::Vector2f(max.x, min.y);
            sf::Vector2f BottomLeft = sf::Vector2f(min.x, max.y);
            sf::Vector2f StartCorner = BottomLeft;
            Graph->SetPosition(StartCorner);
        }
        auto Bound = Graph->GetTotalBound();
        if (TotalBound)
        {
            TotalBound = mergeRects(*TotalBound, Bound);
        }
        else
        {
            TotalBound = Bound;
        }

        sf::Vector2f Offset = sf::Vector2f(0, VerticalSpacing);
        PreBound = TotalBound;
        PreBound->position += Offset;
    }
}

void FDisconnectedGraph::OffsetBy(const sf::Vector2f& InOffset)
{
    for (auto Graph : ConnectedGraphs)
    {
        Graph->OffsetBy(InOffset);
    }
}

std::unordered_map<NodeId, sf::FloatRect> FDisconnectedGraph::GetBoundMap()
{
    std::unordered_map<NodeId, sf::FloatRect> Result;
    for (auto Graph : ConnectedGraphs)
    {
        for (const auto& pair : Graph->GetBoundMap())
        {
            Result[pair.first] = pair.second;
        };
    }
    return Result;
}

FConnectedGraph::FConnectedGraph()
    : FFormatterGraph()
{
}

FConnectedGraph::FConnectedGraph(const FConnectedGraph& Other)
    : FFormatterGraph(Other)
{
}

std::int32_t FConnectedGraph::AssignPathDepthForNodes() const
{
    std::int32_t LongestPath = 1;
    while (true)
    {
        auto Leaves = GetLeavesWithPathDepth0();
        if (Leaves.size() == 0)
        {
            break;
        }
        for (auto leaf : Leaves)
        {
            leaf->PathDepth = LongestPath;
        }
        LongestPath++;
    }
    LongestPath--;
    return LongestPath;
}

FFormatterGraph* FConnectedGraph::Clone()
{
    return new FConnectedGraph(*this);
}

std::vector<FFormatterNode*> FConnectedGraph::GetLeavesWithPathDepth0() const
{
    std::vector<FFormatterNode*> Result;
    for (auto Node : Nodes)
    {
        if (Node->PathDepth != 0 || Node->AnySuccessorPathDepthEqu0())
        {
            continue;
        }
        Result.push_back(Node);
    }
    return Result;
}

void FConnectedGraph::DoLayering()
{
    LayeredList = {};
    std::unordered_set<FFormatterNode*> Set;
    for (std::int32_t i = AssignPathDepthForNodes(); i != 0; i--)
    {
        std::unordered_set<FFormatterNode*> Layer;
        auto Successors = FFormatterNode::GetSuccessorsForNodes(Set);
        auto NodesToProcess = GetNodesGreaterThan(i, Set);
        NodesToProcess.insert(NodesToProcess.end(), Successors.begin(), Successors.end());
        for (auto Node : NodesToProcess)
        {
            auto Predecessors = Node->GetPredecessors();
            bool bPredecessorsFinished = true;
            for (auto Predecessor : Predecessors)
            {
                if (!Set.contains(Predecessor))
                {
                    bPredecessorsFinished = false;
                    break;
                }
            }
            if (bPredecessorsFinished)
            {
                Layer.emplace(Node);
            }
        }
        Set.insert(Layer.begin(), Layer.end());
        std::vector<FFormatterNode*> Array(Layer.begin(), Layer.end());
        if (MaxLayerNodes)
        {
            std::vector<FFormatterNode*> SubLayer;
            for (int j = 0; j != Array.size(); j++)
            {
                SubLayer.push_back(Array[j]);
                if (SubLayer.size() == MaxLayerNodes || j == Array.size() - 1)
                {
                    if (NodeComparer)
                    {
                        std::ranges::sort(SubLayer, NodeComparer);
                    }
                    LayeredList.push_back(SubLayer);
                    SubLayer = {};
                }
            }
        }
        else
        {
            if (NodeComparer)
            {
                std::ranges::sort(Array, NodeComparer);
            }
            LayeredList.push_back(Array);
        }
    }
}

void FConnectedGraph::AddDummyNodes()
{
    for (int i = 0; i < LayeredList.size() - 1; i++)
    {
        auto& Layer = LayeredList[i];
        auto& NextLayer = LayeredList[i + 1];
        for (auto Node : Layer)
        {
            std::vector<FFormatterEdge*> LongEdges;
            for (auto Edge : Node->OutEdges)
            {
                if (!std::ranges::contains(NextLayer, Edge->To->OwningNode))
                {
                    LongEdges.push_back(Edge);
                }
            }
            for (auto Edge : LongEdges)
            {
                auto dummyNode = FFormatterNode::CreateDummy();
                AddNode(dummyNode);
                Node->Connect(Edge->From, dummyNode->InPins[0], Edge->Weight);
                dummyNode->Connect(dummyNode->InPins[0], Edge->From, Edge->Weight);
                dummyNode->Connect(dummyNode->OutPins[0], Edge->To, Edge->Weight);
                Edge->To->OwningNode->Disconnect(Edge->To, Edge->From);
                Edge->To->OwningNode->Connect(Edge->To, dummyNode->OutPins[0], Edge->Weight);
                Node->Disconnect(Edge->From, Edge->To);
                NextLayer.push_back(dummyNode);
            }
        }
    }
}

void FConnectedGraph::SortInLayer(std::vector<std::vector<FFormatterNode*>>& Order, EFormatterPinDirection Direction)
{
    if (Order.size() < 2)
    {
        return;
    }
    const int Start = Direction == EFormatterPinDirection::Out ? Order.size() - 2 : 1;
    const int End = Direction == EFormatterPinDirection::Out ? -1 : Order.size();
    const int Step = Direction == EFormatterPinDirection::Out ? -1 : 1;
    for (int i = Start; i != End; i += Step)
    {
        auto& FixedLayer = Order[i - Step];
        auto& FreeLayer = Order[i];
        for (FFormatterNode* Node : FreeLayer)
        {
            Node->OrderValue = Node->CalcBarycenter(FixedLayer, Direction);
        }
        std::ranges::stable_sort(FreeLayer, [](const FFormatterNode* A, const FFormatterNode* B) -> bool
                             { return A->OrderValue < B->OrderValue; });
        FFormatterNode::CalculatePinsIndexInLayer(FreeLayer);
    }
}

std::vector<FFormatterEdge*> FFormatterNode::GetEdgeBetweenTwoLayer(const std::vector<FFormatterNode*>& LowerLayer, const std::vector<FFormatterNode*>& UpperLayer, const FFormatterNode* ExcludedNode)
{
    std::vector<FFormatterEdge*> Result;
    for (auto Node : LowerLayer)
    {
        if (ExcludedNode == Node)
        {
            continue;
        }
        auto newNodes = Node->GetEdgeLinkedToLayer(UpperLayer, EFormatterPinDirection::In);
        Result.insert(Result.end(), newNodes.begin(), newNodes.end());
    }
    return Result;
}

std::int32_t FFormatterNode::CalculateCrossing(const std::vector<std::vector<FFormatterNode*>>& Order)
{
    CalculatePinsIndex(Order);
    std::int32_t CrossingValue = 0;
    for (int i = 1; i < Order.size(); i++)
    {
        const auto& UpperLayer = Order[i - 1];
        const auto& LowerLayer = Order[i];
        std::vector<FFormatterEdge*> NodeEdges = GetEdgeBetweenTwoLayer(LowerLayer, UpperLayer);
        while (NodeEdges.size() != 0)
        {
            const auto Edge1 = NodeEdges.back();
            NodeEdges.pop_back();
            for (const auto Edge2 : NodeEdges)
            {
                if (Edge1->IsCrossing(Edge2))
                {
                    CrossingValue++;
                }
            }
        }
    }
    return CrossingValue;
}

void FConnectedGraph::DoOrderingSweep()
{
    auto Best = LayeredList;
    auto Order = LayeredList;
    std::int32_t BestCrossing = INT_MAX;
    for (int i = 0; i < MaxOrderingIterations; i++)
    {
        SortInLayer(Order, i % 2 == 0 ? EFormatterPinDirection::In : EFormatterPinDirection::Out);
        const std::int32_t NewCrossing = FFormatterNode::CalculateCrossing(Order);
        if (NewCrossing < BestCrossing)
        {
            Best = Order;
            BestCrossing = NewCrossing;
        }
    }
    LayeredList = Best;
}

void FConnectedGraph::DoPositioning()
{
    if (PositioningAlgorithm == EGraphFormatterPositioningAlgorithm::EEvenlyInLayer)
    {
        FEvenlyPlaceStrategy LeftToRightPositioningStrategy(LayeredList);
        TotalBound = LeftToRightPositioningStrategy.GetTotalBound();
    }
    else if (PositioningAlgorithm == EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodMedian || PositioningAlgorithm == EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodTop)
    {
        FFastAndSimplePositioningStrategy FastAndSimplePositioningStrategy(LayeredList, true);
        TotalBound = FastAndSimplePositioningStrategy.GetTotalBound();
    }
    else if (PositioningAlgorithm == EGraphFormatterPositioningAlgorithm::ELayerSweep)
    {
        FPriorityPositioningStrategy PriorityPositioningStrategy(LayeredList);
        TotalBound = PriorityPositioningStrategy.GetTotalBound();
    }
}

std::unordered_map<PinId, sf::Vector2f> FConnectedGraph::GetPinsOffset()
{
    std::unordered_map<PinId, sf::Vector2f> Result;
    for (auto Node : Nodes)
    {
        for (auto OutPin : Node->OutPins)
        {
            sf::Vector2f PinOffset = Node->Position + OutPin->NodeOffset - TotalBound->position;
            Result.emplace(OutPin->OriginalPin, PinOffset);
        }
        for (auto InPin : Node->InPins)
        {
            sf::Vector2f PinOffset = Node->Position + InPin->NodeOffset - TotalBound->position;
            Result.emplace(InPin->OriginalPin, PinOffset);
        }
    }
    return Result;
}

std::vector<FFormatterPin*> FConnectedGraph::GetInputPins() const
{
    std::unordered_set<FFormatterPin*> Result;
    for (auto Node : Nodes)
    {
        for (auto Pin : Node->InPins)
        {
            Result.emplace(Pin);
        }
    }
    return std::vector<FFormatterPin*>(Result.begin(), Result.end());
}

std::vector<FFormatterPin*> FConnectedGraph::GetOutputPins() const
{
    std::unordered_set<FFormatterPin*> Result;
    for (auto Node : Nodes)
    {
        for (auto Pin : Node->OutPins)
        {
            Result.emplace(Pin);
        }
    }
    return std::vector<FFormatterPin*>(Result.begin(), Result.end());
}

std::unordered_set<NodeId> FConnectedGraph::GetOriginalNodes() const
{
    std::unordered_set<NodeId> Result;
    for (auto Node : Nodes)
    {
        if (SubGraphs.contains(Node->Guid))
        {
            auto newNodes = SubGraphs.at(Node->Guid)->GetOriginalNodes();
            Result.insert(newNodes.begin(), newNodes.end());
        }
        if (Node->OriginalNode)
        {
            Result.emplace(Node->OriginalNode);
        }
    }
    return Result;
}

void FConnectedGraph::Format()
{
    for (auto [Key, SubGraph] : SubGraphs)
    {
        auto Node = NodesMap[Key];
        SubGraph->Format();
        auto SubGraphBorder = SubGraph->GetBorder();
        Node->UpdatePinsOffset(SubGraphBorder.position);
        auto Bound = SubGraph->GetTotalBound();
        Node->InitPosition(Bound.position - SubGraphBorder.position);

        const auto min = SubGraphBorder.position;
        const auto max = SubGraphBorder.position + SubGraphBorder.size;
        Node->Size = SubGraph->GetTotalBound().size + sf::Vector2f(min.x + max.x, min.y + max.y);
    }
    if (Nodes.size() > 0)
    {
        RemoveCycle();
        DoLayering();
        AddDummyNodes();
        if (!NodeComparer)
        {
            DoOrderingSweep();
        }
        DoPositioning();
    }
}

void FConnectedGraph::OffsetBy(const sf::Vector2f& InOffset)
{
    for (auto Node : Nodes)
    {
        Node->SetPosition(Node->GetPosition() + InOffset);
    }
    TotalBound->position += InOffset;
}

std::unordered_map<NodeId, sf::FloatRect> FConnectedGraph::GetBoundMap()
{
    std::unordered_map<NodeId, sf::FloatRect> Result;
    for (auto Node : Nodes)
    {
        if (!Node->OriginalNode)
        {
            continue;
        }
        Result.emplace(Node->OriginalNode, sf::FloatRect(Node->GetPosition(), Node->Size));
        if (SubGraphs.contains(Node->Guid))
        {
            for (const auto& pair : SubGraphs[Node->Guid]->GetBoundMap())
            {
                Result[pair.first] = pair.second;
            };
        }
    }
    return Result;
}

