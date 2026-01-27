#pragma once

#include "graph-utils.hpp"
#include "graph.hpp"

#include <algorithm>
#include <unordered_set>
#include <vector>

class Formatter
{
public:
    Graph* graph{nullptr};

    std::unordered_set<NodeId> GetStarterNodes()
    {
        std::unordered_set<NodeId> nodes = GraphUtils::getSelectedNodes();
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
            nodes = GraphUtils::getLinkedNodes(*graph, nodes);
        }

        return nodes;
    }

    std::vector<std::vector<NodeId>> arrangeSubgraph(const std::unordered_set<NodeId>& nodes)
    {
        std::unordered_map<NodeId, int> nodeToLayer;
        auto assignLayer = [&](auto& func, NodeId node, int layer) -> void
        {
            auto it = nodeToLayer.find(node);
            if ((it == nodeToLayer.end()) || (layer > it->second))
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

        auto roots = GraphUtils::getRootNodes(*graph, nodes);
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
                    fixedNodes.merge(GraphUtils::getPredecessor(*graph, {node}));
                    continue;
                }

                int smallestGap = -1;
                for (auto pred : GraphUtils::getPredecessor(*graph, {node}))
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
                    for (auto n : GraphUtils::getPredecessor(*graph, {node}))
                    {
                        if (fixedNodes.contains(n))
                        {
                            continue;
                        }

                        nodeToLayer[n] += smallestGap - 1;
                    }
                }

                fixedNodes.merge(GraphUtils::getPredecessor(*graph, {node}));
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
        auto subgraphs = GraphUtils::findSubgraphs(*graph, nodes);

        const float verticalSpacing = 50.f;
        const float horizontalSpacing = 50.f;

        sf::Vector2f origin{};

        bool isFirstSubGraph = true;

        for (const auto& subgraph : subgraphs)
        {
            sf::FloatRect oldBound;
            if (isFirstSubGraph)
            {
                oldBound = GraphUtils::getNodesBound(subgraph);
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
                auto newBound = GraphUtils::getNodesBound(subgraph);

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
