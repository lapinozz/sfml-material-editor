#pragma once

#include <unordered_set>
#include <vector>
#include <algorithm>

#include "graph.hpp"

template<typename T>
sf::Rect<T> mergeRects(const sf::Rect<T>& r1, const sf::Rect<T>& r2)
{
    const auto minX = std::min(r1.position.x, r2.position.x);
    const auto minY = std::min(r1.position.y, r2.position.y);
    const auto maxX = std::max(r1.position.x + r1.size.x, r2.position.x + r2.size.x);
    const auto maxY = std::max(r1.position.y + r1.size.y, r2.position.y + r2.size.y);

    return { {minX, minY}, {maxX - minX, maxY - minY} };
}

template<typename T>
sf::Rect<T> mergeRects(const std::optional<sf::Rect<T>>& r1, const sf::Rect<T>& r2)
{
    if (!r1)
    {
        return r2;
    }

    const auto minX = std::min(r1->position.x, r2.position.x);
    const auto minY = std::min(r1->position.y, r2.position.y);
    const auto maxX = std::max(r1->position.x + r1->size.x, r2.position.x + r2.size.x);
    const auto maxY = std::max(r1->position.y + r1->size.y, r2.position.y + r2.size.y);

    return { {minX, minY}, {maxX - minX, maxY - minY} };
}

sf::FloatRect getNodeBounds(NodeId node)
{
    sf::Vector2f Pos = UEGraphAdapter::GetNodePosition(node);
    sf::Vector2f Size = UEGraphAdapter::GetNodeSize(node);
    return { Pos, Size };
}

template<typename T>
sf::FloatRect getNodesBound(const T& nodes)
{
    std::optional<sf::FloatRect> Bound;
    for (auto node : nodes)
    {
        Bound = mergeRects(Bound, getNodeBounds(node));
    }
    return Bound ? *Bound : sf::FloatRect{};
}

class Formatter
{
public:
    Graph* graph{ nullptr };

    std::unordered_set<NodeId> GetSelectedNodes()
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

    std::unordered_set<NodeId> GetLinkedNodes(const std::unordered_set<NodeId>& rootNodes)
    {
        std::unordered_set<NodeId> visitedNodes;
        std::unordered_set<NodeId> pendingNodes = rootNodes;
        while (pendingNodes.size() > 0)
        {
            NodeId Node = *pendingNodes.begin();
            pendingNodes.erase(Node);
            visitedNodes.emplace(Node);

            for (const auto link : graph->links)
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
                    if (!visitedNodes.contains(linkedPin.nodeId()))
                    {
                        pendingNodes.emplace(linkedPin.nodeId());
                    }
                }
            }
        }
        return visitedNodes;
    }

    std::unordered_set<NodeId> getPredecessor(const std::unordered_set<NodeId>& rootNodes)
    {
        std::unordered_set<NodeId> visitedNodes;
        std::unordered_set<NodeId> pendingNodes = rootNodes;
        while (pendingNodes.size() > 0)
        {
            NodeId Node = *pendingNodes.begin();
            pendingNodes.erase(Node);
            visitedNodes.emplace(Node);

            for (const auto link : graph->links)
            {
                if (link.to().nodeId() == Node)
                {
                    const auto linkedNode = link.from().nodeId();
                    if (!visitedNodes.contains(linkedNode))
                    {
                        pendingNodes.emplace(linkedNode);
                    }
                }
            }
        }
        return visitedNodes;
    }

    std::unordered_set<NodeId> GetStarterNodes()
    {
        std::unordered_set<NodeId> nodes = GetSelectedNodes();
        if (nodes.size() == 0)
        {
            for (const auto& Node : graph->nodes)
            {
                nodes.emplace(Node->id);
                ed::SelectNode(Node->id, true);
            }
        }
        else
        {
            nodes = GetLinkedNodes(nodes);
        }

        return nodes;
    }

    std::unordered_set<NodeId> getRootNodes(const std::unordered_set<NodeId>& nodes)
    {
        std::unordered_set<NodeId> roots;
        for (const auto& node : nodes)
        {
            bool hasOutLink = false;
            for (const auto link : graph->links)
            {
                if (link.from().nodeId() == node)
                {
                    hasOutLink = true;
                    break;
                }
            }

            if (!hasOutLink)
            {
                roots.emplace(node);
            }
        }

        return roots;
    }

    std::vector<std::unordered_set<NodeId>> findSubgraphs(const std::unordered_set<NodeId>& Nodes)
    {
        std::vector<std::unordered_set<NodeId>> Result;
        std::unordered_set<NodeId> CheckedNodes;
        std::vector<NodeId> Stack;
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
                NodeId Top = Stack.back();
                Stack.erase(Stack.end() - 1);

                IsolatedNodes.emplace(Top);

                for (auto ConnectedNode : GetLinkedNodes({ Top }))
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

    std::vector<std::vector<NodeId>> arrangeSubgraph(const std::unordered_set<NodeId>& nodes)
    {
        std::unordered_map<NodeId, int> nodeToLayer;
        auto assignLayer = [&](auto& func, NodeId node, int layer) -> void
        {
            auto it = nodeToLayer.find(node);
            if((it == nodeToLayer.end()) || (layer > it->second))
            {
                nodeToLayer[node] = layer;

                for (const auto link : graph->links)
                {
                    if (link.to().nodeId() == node)
                    {
                        func(func, link.from().nodeId(), layer + 1);
                    }
                }
            }
        };

        auto roots = getRootNodes(nodes);
        for (auto node : roots)
        {
            assignLayer(assignLayer, node, 0);
        }

        bool hasChange;
        do
        {
            hasChange = false;

            std::unordered_set<NodeId> fixedNodes;
            for (auto& node : roots)
            {
                if (fixedNodes.size() == 0)
                {
                    fixedNodes.merge(getPredecessor({ node }));
                    continue;
                }

                int smallestGap = -1;
                for (auto pred : getPredecessor({ node }))
                {
                    if (fixedNodes.contains(pred))
                    {
                        continue;
                    }

                    for (const auto link : graph->links)
                    {
                        if (link.to().nodeId() == pred)
                        {
                            const auto linkNode = link.from().nodeId();
                            if (!fixedNodes.contains(linkNode))
                            {
                                continue;
                            }

                            int gap = nodeToLayer[linkNode] - nodeToLayer[pred];
                            if (smallestGap == -1 || gap < smallestGap)
                            {
                                smallestGap = gap;
                            }
                        }
                    }
                }

                if (smallestGap > 1)
                {
                    hasChange = true;
                    for (auto n : getPredecessor({ node }))
                    {
                        if (fixedNodes.contains(n))
                        {
                            continue;
                        }

                        nodeToLayer[n] += smallestGap - 1;
                    }
                }

                fixedNodes.merge(getPredecessor({ node }));
            }
             
        } while (hasChange);

        int maxLayer{};
        for (const auto& pair : nodeToLayer)
        {
            maxLayer = std::max(maxLayer, pair.second);
        }

        std::vector<std::vector<NodeId>> layerToNode(maxLayer + 1);
        for (const auto& pair : nodeToLayer)
        {
            layerToNode[pair.second].push_back(pair.first);
        }

        return layerToNode;
    }

    void format()
    {
        auto nodes = GetStarterNodes();
        auto subgraphs = findSubgraphs(nodes);

        const float verticalSpacing = 50.f;
        const float horizontalSpacing = 50.f;

        sf::Vector2f origin{};

        bool isFirstSubGraph = true;

        for (const auto& subgraph : subgraphs)
        {
            sf::FloatRect oldBound;
            if (isFirstSubGraph)
            {
                oldBound = getNodesBound(subgraph);
            }

            float maxLayerHeight{};

            sf::Vector2f layerOrigin = origin;

            auto layers = arrangeSubgraph(subgraph);

            for (int layerIndex = 0; layerIndex < layers.size(); layerIndex++)
            {
                const auto layer = layers[layerIndex];
                float maxNodeWidth{};
                for (auto node : layer)
                {
                    maxNodeWidth = std::max(maxNodeWidth, ed::GetNodeSize(node).x);
                }

                layerOrigin.x -= maxNodeWidth;

                for (int nodeIndex = 0; nodeIndex < layer.size(); nodeIndex++)
                {
                    auto node = layer[nodeIndex];
                    ed::SetNodePosition(node, layerOrigin);
                    layerOrigin.y += verticalSpacing;
                    layerOrigin.y += ed::GetNodeSize(node).y;
                }

                maxLayerHeight = std::max(maxLayerHeight, layerOrigin.y);

                layerOrigin.y = origin.y;
                layerOrigin.x -= horizontalSpacing;
            }

            origin.y += maxLayerHeight;

            if (isFirstSubGraph)
            {
                auto newBound = getNodesBound(subgraph);

                auto oldCenter = oldBound.position + oldBound.size / 2.f;
                auto newCenter = newBound.position + newBound.size / 2.f;

                auto offset = oldCenter - newCenter;
                origin += offset;

                for (const auto& layer : layers)
                {
                    for (auto node : layer)
                    {
                        ed::SetNodePosition(node, ed::GetNodePosition(node) + offset);
                    }
                }
            }

            isFirstSubGraph = false;
        }
    }
};


