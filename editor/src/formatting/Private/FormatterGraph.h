/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Howaajin. All rights reserved.
 *  Licensed under the MIT License. See License in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#pragma once

#include <SFML/Graphics.hpp>

#include <unordered_set>
#include "graph.hpp"

#include "guid.hpp"

class FConnectedGraph;
class FFormatterNode;
class FFormatterGraph;

enum class EGraphFormatterPositioningAlgorithm
{
    EEvenlyInLayer,
    EFastAndSimpleMethodTop,
    EFastAndSimpleMethodMedian,
    ELayerSweep,
};

enum class EFormatterPinDirection
{
    In,
    Out,
    InOut,
};

class FFormatterPin
{
public:
    // Guid for this Pin, used to mapping different instances to one instance
    // For example, in a duplicated graph, needs to know the original pin
    FGuid Guid;

    // User-defined pointer used as a mapping key from a child graph pin to the original graph pin
    PinId OriginalPin{0};
    
    EFormatterPinDirection Direction{EFormatterPinDirection::In};

    // Owner of the pin
    FFormatterNode* OwningNode{nullptr};

    // Position relate to Owner
    sf::Vector2f NodeOffset;
    
    // When the "Owner" is assigned to a certain layer, this pin's index within that layer
    // Can be used to detect intersections
    std::int32_t IndexInLayer{-1};
};

// An edge is defined here as belonging to a specific node,
// meaning the connection between two nodes, with each edge storing information about the connection between the two nodes
class FFormatterEdge
{
public:
    // The owner's pin the edge linked to, it can be either an "In" or "Out" direction
    FFormatterPin* From;

    // Indicates which pin this edge is connected to, it can be either an "In" or "Out" direction
    FFormatterPin* To;
    float Weight = 1;
    bool IsCrossing(const FFormatterEdge* Edge) const;
    bool IsInnerSegment() const;
};

class FFormatterNode
{
public:
    static FFormatterNode* CreateDummy();
    static std::vector<FFormatterEdge*> GetEdgeBetweenTwoLayer(const std::vector<FFormatterNode*>& LowerLayer, const std::vector<FFormatterNode*>& UpperLayer, const FFormatterNode* ExcludedNode = nullptr);
    static std::vector<FFormatterNode*> GetSuccessorsForNodes(std::unordered_set<FFormatterNode*> Nodes);
    static void CalculatePinsIndexInLayer(const std::vector<FFormatterNode*>& Layer);
    static void CalculatePinsIndex(const std::vector<std::vector<FFormatterNode*>>& Order);
    static std::int32_t CalculateCrossing(const std::vector<std::vector<FFormatterNode*>>& Order);

    // Guid for this node, used to mapping different instances to one instance
    // For example, in a duplicated graph, needs to know the original pin
    FGuid Guid;

    // Custom pointer for user-defined nodes tracking
    NodeId OriginalNode{ 0 };

    // Indicating whether this node is a subgraph
    FFormatterGraph* SubGraph = nullptr;

    // Representing the size of the node or the bounding box size of a subgraph
    sf::Vector2f Size;

    // Edges with an "In" direction "From" pin
    std::vector<FFormatterEdge*> InEdges;
    
    // Edges with an "Out" direction "From" pin
    std::vector<FFormatterEdge*> OutEdges;

    // All pins with an "In" direction
    std::vector<FFormatterPin*> InPins;
    
    // All pins with an "Out" direction
    std::vector<FFormatterPin*> OutPins;

    // Path depth used in the longest path layering algorithm 
    std::int32_t PathDepth;
    
    // Positioning priority used in priority positioning algorithm 
    std::int32_t PositioningPriority;
    
    FFormatterNode(const FFormatterNode& Other);
    FFormatterNode();

    ~FFormatterNode();
    void Connect(FFormatterPin* SourcePin, FFormatterPin* TargetPin, float Weight = 1);
    void Disconnect(FFormatterPin* SourcePin, FFormatterPin* TargetPin);
    void AddPin(FFormatterPin* Pin);
    std::vector<FFormatterNode*> GetSuccessors() const;
    std::vector<FFormatterNode*> GetPredecessors() const;
    bool IsSource() const;
    bool IsSink() const;
    bool AnySuccessorPathDepthEqu0() const;
    bool IsCrossingInnerSegment(const std::vector<FFormatterNode*>& LowerLayer, const std::vector<FFormatterNode*>& UpperLayer) const;
    FFormatterNode* GetMedianUpper() const;
    FFormatterNode* GetMedianLower() const;
    std::vector<FFormatterNode*> GetUppers() const;
    std::vector<FFormatterNode*> GetLowers() const;
    std::int32_t GetInputPinCount() const;
    std::int32_t GetInputPinIndex(FFormatterPin* InputPin) const;
    std::int32_t GetOutputPinCount() const;
    std::int32_t GetOutputPinIndex(FFormatterPin* OutputPin) const;
    std::vector<FFormatterEdge*> GetEdgeLinkedToLayer(const std::vector<FFormatterNode*>& Layer, EFormatterPinDirection Direction) const;
    float CalcBarycenter(const std::vector<FFormatterNode*>& Layer, EFormatterPinDirection Direction) const;
    float GetLinkedPositionToNode(const FFormatterNode* Node, EFormatterPinDirection Direction, bool IsHorizontalDirection = true);
    float GetMaxWeight(EFormatterPinDirection Direction);
    float GetMaxWeightToNode(const FFormatterNode* Node, EFormatterPinDirection Direction);
    std::int32_t CalcPriority(EFormatterPinDirection Direction) const;
    void InitPosition(sf::Vector2f InPosition);
    void SetPosition(sf::Vector2f InPosition);
    sf::Vector2f GetPosition() const;
    void SetSubGraph(FFormatterGraph* InSubGraph);
    void UpdatePinsOffset(sf::Vector2f Border);
    friend class FConnectedGraph;

private:
    float OrderValue{0.0f};
    sf::Vector2f Position;
};

class FFormatterGraph
{
public:
    static std::vector<sf::FloatRect> CalculateLayersBound(std::vector<std::vector<FFormatterNode*>>& InLayeredNodes, bool IsHorizontalDirection = true);
    inline static std::int32_t HorizontalSpacing = 100;
    inline static std::int32_t VerticalSpacing = 80;
    inline static sf::Vector2f SpacingFactorOfGroup = sf::Vector2f(0.314f, 0.314f);
    inline static std::int32_t MaxLayerNodes = 5;
    inline static std::int32_t MaxOrderingIterations = 10;
    inline static EGraphFormatterPositioningAlgorithm PositioningAlgorithm = EGraphFormatterPositioningAlgorithm::EFastAndSimpleMethodTop;

    FFormatterGraph(const FFormatterGraph& Other);
    virtual ~FFormatterGraph();
    void DetachAndDestroy();
    virtual FFormatterGraph* Clone();

    explicit FFormatterGraph();

    void AddNode(FFormatterNode* InNode);

    virtual std::unordered_map<PinId, sf::Vector2f> GetPinsOffset() { return {}; }

    virtual std::vector<FFormatterPin*> GetInputPins() const { return {}; }

    virtual std::vector<FFormatterPin*> GetOutputPins() const { return {}; }

    virtual std::unordered_set<NodeId> GetOriginalNodes() const { return {}; }

    virtual void Format() { }
    
    virtual void OffsetBy(const sf::Vector2f& InOffset) { }
    virtual void SetPosition(const sf::Vector2f& Position);

    virtual std::unordered_map<NodeId, sf::FloatRect> GetBoundMap() { return {}; }

    sf::FloatRect GetTotalBound() const { return *TotalBound; }

    void SetBorder(float Left, float Top, float Right, float Bottom);
    sf::FloatRect GetBorder() const;
    bool (*NodeComparer)(const FFormatterNode* A, const FFormatterNode* B) = nullptr;
    std::unordered_map<PinId, FFormatterPin*> OriginalPinsMap;
    std::vector<FFormatterNode*> Nodes;

protected:
    std::unordered_map<FGuid, FFormatterNode*> NodesMap;
    std::unordered_map<FGuid, FFormatterGraph*> SubGraphs;
    std::unordered_map<FGuid, FFormatterPin*> PinsMap;
    
    std::optional<sf::FloatRect> TotalBound;
    std::optional<sf::FloatRect > Border;
private:
    bool IsNodeDetached = false;
};

class FDisconnectedGraph : public FFormatterGraph
{
    std::vector<FFormatterGraph*> ConnectedGraphs;

public:
    void AddGraph(FFormatterGraph* Graph);
    virtual ~FDisconnectedGraph() override;
    virtual std::unordered_map<PinId, sf::Vector2f> GetPinsOffset() override;
    virtual std::vector<FFormatterPin*> GetInputPins() const override;
    virtual std::vector<FFormatterPin*> GetOutputPins() const override;
    virtual std::unordered_set<NodeId> GetOriginalNodes() const override;
    virtual void Format() override;
    virtual void OffsetBy(const sf::Vector2f& InOffset) override;
    virtual std::unordered_map<NodeId, sf::FloatRect> GetBoundMap() override;
};

class FConnectedGraph : public FFormatterGraph
{
public:
    FConnectedGraph();
    FConnectedGraph(const FConnectedGraph& Other);
    virtual FFormatterGraph* Clone() override;
    void RemoveNode(FFormatterNode* NodeToRemove);
    virtual void Format() override;
    virtual std::unordered_map<NodeId, sf::FloatRect> GetBoundMap() override;
    virtual void OffsetBy(const sf::Vector2f& InOffset) override;
    virtual std::unordered_map<PinId, sf::Vector2f> GetPinsOffset() override;
    virtual std::vector<FFormatterPin*> GetInputPins() const override;
    virtual std::vector<FFormatterPin*> GetOutputPins() const override;
    virtual std::unordered_set<NodeId> GetOriginalNodes() const override;

private:
    
    std::vector<FFormatterNode*> GetNodesGreaterThan(std::int32_t i, std::unordered_set<FFormatterNode*>& Excluded);

    void DoLayering();
    void RemoveCycle();
    void AddDummyNodes();
    void SortInLayer(std::vector<std::vector<FFormatterNode*>>& Order, EFormatterPinDirection Direction);
    void DoOrderingSweep();
    void DoPositioning();
    FFormatterNode* FindSourceNode() const;
    FFormatterNode* FindSinkNode() const;
    FFormatterNode* FindMaxInOutWeightDiffNode() const;
    std::vector<FFormatterNode*> GetLeavesWithPathDepth0() const;
    std::int32_t AssignPathDepthForNodes() const;

    std::vector<std::vector<FFormatterNode*>> LayeredList;
};

