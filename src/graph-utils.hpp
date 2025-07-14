#pragma once

#include <unordered_set>

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

namespace GraphUtils
{
    std::unordered_set<NodeId> getSelectedNodes()
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

    sf::FloatRect getNodeBounds(NodeId node)
    {
        sf::Vector2f Pos = UEGraphAdapter::GetNodePosition(node);
        sf::Vector2f Size = UEGraphAdapter::GetNodeSize(node);
        return { Pos, Size };
    }

    sf::FloatRect getNodeBounds(const Graph::Node::Ptr& node)
    {
        return getNodeBounds(node->id);
    }

    template<typename T>
    sf::FloatRect getNodesBound(const T& nodes)
    {
        std::optional<sf::FloatRect> Bound;
        for (const auto& node : nodes)
        {
            Bound = mergeRects(Bound, getNodeBounds(node));
        }
        return Bound ? *Bound : sf::FloatRect{};
    }

    std::unordered_set<NodeId> getLinkedNodes(const Graph& graph, const std::unordered_set<NodeId>& rootNodes)
    {
        std::unordered_set<NodeId> visitedNodes;
        std::unordered_set<NodeId> pendingNodes = rootNodes;
        while (pendingNodes.size() > 0)
        {
            NodeId Node = *pendingNodes.begin();
            pendingNodes.erase(Node);
            visitedNodes.emplace(Node);

            for (const auto link : graph.links)
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

    std::unordered_set<NodeId> getPredecessor(const Graph& graph, const std::unordered_set<NodeId>& rootNodes)
    {
        std::unordered_set<NodeId> visitedNodes;
        std::unordered_set<NodeId> pendingNodes = rootNodes;
        while (pendingNodes.size() > 0)
        {
            NodeId Node = *pendingNodes.begin();
            pendingNodes.erase(Node);
            visitedNodes.emplace(Node);

            for (const auto link : graph.links)
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

    std::unordered_set<NodeId> getRootNodes(const Graph& graph, const std::unordered_set<NodeId>& nodes)
    {
        std::unordered_set<NodeId> roots;
        for (const auto& node : nodes)
        {
            bool hasOutLink = false;
            for (const auto link : graph.links)
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

    std::vector<std::unordered_set<NodeId>> findSubgraphs(const Graph& graph, const std::unordered_set<NodeId>& Nodes)
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

                for (auto ConnectedNode : getLinkedNodes(graph, { Top }))
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
}