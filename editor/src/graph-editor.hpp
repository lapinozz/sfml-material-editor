#pragma once

#include "graph.hpp"
#include "misc/cpp/imgui_stdlib.h"

// Nodes
#include "nodes/archetypes.hpp"
#include "nodes/bridge.hpp"
#include "nodes/expression.hpp"
#include "nodes/output.hpp"
#include "nodes/parameter.hpp"

//ImGui
#include "code-generator.hpp"

struct GraphEditor
{
    Graph& graph;
    GraphContext& graphContext;

    ArchetypeRepo& archetypes;

    std::vector<PinId> newNodeTargetPins;
    bool newNodeFilterType = true;

    bool canTarget(ValueType outType, ValueType inType)
    {
        if (canConvert(outType, inType))
        {
            return true;
        }

        if (!outType && inType.isGenType())
        {
            return true;
        }

        if (outType.isGenType() && !inType)
        {
            return true;
        }

        return false;
    }

    bool canTarget(PinId out, PinId in)
    {
        if (in == out)
        {
            return false;
        }

        if (in.direction() == out.direction())
        {
            return false;
        }

        if (in.nodeId() == out.nodeId())
        {
            return false;
        }

        if (graph.hasLink({in, out}))
        {
            return false;
        }

        if (in.direction() == PinDirection::Out)
        {
            std::swap(in, out);
        }

        const auto& outNode = graph.getNode<ExpressionNode>(out.nodeId());
        const auto& outArchetype = *outNode.archetype;
        const auto outIndex = out.index();
        const auto* outBridge = graph.findNode<BridgeNode>(out.nodeId());

        const auto& inNode = graph.getNode<ExpressionNode>(in.nodeId());
        const auto& inArchetype = *inNode.archetype;
        const auto inputIndex = in.index();
        const auto* inBridge = graph.findNode<BridgeNode>(in.nodeId());

        if (inBridge || outBridge)
        {
            return true;
        }

        if (canTarget(outNode.outputs[outIndex].type, inNode.inputs[inputIndex].type))
        {
            return true;
        }

        for (const auto& inOverload : inArchetype.overloads)
        {
            if (canTarget(outNode.outputs[outIndex].type, inOverload.inputs[inputIndex]))
            {
                return true;
            }
        }

        for (const auto& outOverload : outArchetype.overloads)
        {
            if (canTarget(outOverload.outputs[outIndex], inNode.inputs[inputIndex].type))
            {
                return true;
            }

            for (const auto& inOverload : inArchetype.overloads)
            {
                if (canTarget(outOverload.outputs[outIndex], inOverload.inputs[inputIndex]))
                {
                    return true;
                }
            }
        }

        return false;
    }

    int findConnectTarget(const NodeArchetype& out, PinId in)
    {
        const auto& inNode = graph.getNode<ExpressionNode>(in.nodeId());
        const auto& inArchetype = *inNode.archetype;
        const auto inputIndex = in.index();
        const auto* inBridge = graph.findNode<BridgeNode>(in.nodeId());

        if (inBridge)
        {
            return in.nodeId().makeInput(0);
        }

        for (int outIndex{}; outIndex < out.outputs.size(); outIndex++)
        {
            if (canTarget(out.outputs[outIndex].type, inNode.inputs[inputIndex].type))
            {
                return outIndex;
            }

            for (const auto& inOverload : inArchetype.overloads)
            {
                if (canTarget(out.outputs[outIndex].type, inOverload.inputs[inputIndex]))
                {
                    return outIndex;
                }
            }
        }

        for (const auto& outOverload : out.overloads)
        {
            for (int outIndex{}; outIndex < outOverload.outputs.size(); outIndex++)
            {
                if (canTarget(outOverload.outputs[outIndex], inNode.inputs[inputIndex].type))
                {
                    return outIndex;
                }

                for (const auto& inOverload : inArchetype.overloads)
                {
                    if (canTarget(outOverload.outputs[outIndex], inOverload.inputs[inputIndex]))
                    {
                        return outIndex;
                    }
                }
            }
        }

        return -1;
    }

    int findConnectTarget(PinId out, const NodeArchetype& in)
    {
        const auto& outNode = graph.getNode<ExpressionNode>(out.nodeId());
        const auto& outArchetype = *outNode.archetype;
        const auto outIndex = out.index();
        const auto* outBridge = graph.findNode<BridgeNode>(out.nodeId());

        if (outBridge)
        {
            return out.nodeId().makeOutput(0);
        }

        for (int inputIndex{}; inputIndex < in.inputs.size(); inputIndex++)
        {
            if (canTarget(outNode.outputs[outIndex].type, in.inputs[inputIndex].type))
            {
                return inputIndex;
            }
        }

        for (const auto& inOverload : in.overloads)
        {
            for (int inputIndex{}; inputIndex < inOverload.inputs.size(); inputIndex++)
            {
                if (canTarget(outNode.outputs[outIndex].type, inOverload.inputs[inputIndex]))
                {
                    return inputIndex;
                }
            }
        }

        for (const auto& outOverload : outArchetype.overloads)
        {
            for (int inputIndex{}; inputIndex < in.inputs.size(); inputIndex++)
            {
                if (canTarget(outOverload.outputs[outIndex], in.inputs[inputIndex].type))
                {
                    return inputIndex;
                }
            }

            for (const auto& inOverload : in.overloads)
            {
                for (int inputIndex{}; inputIndex < inOverload.inputs.size(); inputIndex++)
                {
                    if (canTarget(outOverload.outputs[outIndex], inOverload.inputs[inputIndex]))
                    {
                        return inputIndex;
                    }
                }
            }
        }

        return -1;
    }

    void updateCreate()
    {
        if (ed::BeginCreate())
        {
            bool firstLink = true;

            ed::PinId edInputPinId, edOutputPinId;
            while (ed::QueryNewLink(&edInputPinId, &edOutputPinId, ImVec4(255, 255, 255, 255)))
            {
                PinId inputPinId{edInputPinId};
                PinId outputPinId{edOutputPinId};

                if (ImGui::GetIO().KeyCtrl)
                {
                    graph.removeLink(LinkId(inputPinId, outputPinId));
                }

                auto* inputBridge = inputPinId ? graph.findNode<BridgeNode>(inputPinId.nodeId()) : nullptr;
                auto* outputBridge = outputPinId ? graph.findNode<BridgeNode>(outputPinId.nodeId()) : nullptr;

                if (!inputPinId || !outputPinId)
                {
                    ed::RejectNewItem();
                }
                else
                {
                    if (inputPinId == outputPinId)
                    {
                        if (inputBridge)
                        {
                            inputBridge->connectionMode = BridgeNode::ConnectionMode::Neutral;
                        }
                    }
                    else if (inputBridge && outputBridge)
                    {
                        inputBridge->connectionMode = BridgeNode::ConnectionMode::Output;
                        outputBridge->connectionMode = BridgeNode::ConnectionMode::Input;
                    }
                    else
                    {
                        if (inputBridge)
                        {
                            inputBridge->connectionMode = outputPinId.direction() == PinDirection::In
                                                              ? BridgeNode::ConnectionMode::Output
                                                              : BridgeNode::ConnectionMode::Input;
                        }

                        if (outputBridge)
                        {
                            outputBridge->connectionMode = inputPinId.direction() == PinDirection::In
                                                               ? BridgeNode::ConnectionMode::Output
                                                               : BridgeNode::ConnectionMode::Input;
                        }
                    }

                    if (outputBridge)
                    {
                        outputPinId = outputBridge->connectionMode == BridgeNode::ConnectionMode::Output
                                          ? outputPinId.nodeId().makeOutput(0)
                                          : outputPinId.nodeId().makeInput(0);
                    }

                    if (inputBridge)
                    {
                        inputPinId = inputBridge->connectionMode == BridgeNode::ConnectionMode::Output
                                         ? inputPinId.nodeId().makeOutput(0)
                                         : inputPinId.nodeId().makeInput(0);
                    }

                    const auto valid = canTarget(inputPinId, outputPinId);
                    if (!valid)
                    {
                        ed::RejectNewItem(ImVec4(255, 0, 0, 255));
                    }
                    else if (ed::AcceptNewItem(ImVec4(255, 255, 255, 255)))
                    {
                        graph.addLink(inputPinId, outputPinId);
                    }
                }

                firstLink = false;
            }

            ed::PinId edPinId = 0;
            while (ed::QueryNewNode(&edPinId))
            {
                PinId pinId{edPinId};

                if (ed::AcceptNewItem(ImVec4(255, 255, 255, 255)))
                {
                    if (auto* bridge = graph.findNode<BridgeNode>(pinId.nodeId()))
                    {
                        if (bridge->connectionMode == BridgeNode::ConnectionMode::Input)
                        {
                            newNodeTargetPins.push_back(pinId.nodeId().makeInput(0));
                        }
                        else
                        {
                            newNodeTargetPins.push_back(pinId.nodeId().makeOutput(0));
                        }
                    }
                    else
                    {
                        newNodeTargetPins.push_back(pinId);
                    }

                    ed::Suspend();
                    ImGui::OpenPopup("Create New Node");
                    ed::Resume();
                }
            }
        }
        ed::EndCreate();
    }

    void updateDelete()
    {
        if (ed::BeginDelete())
        {
            // There may be many links marked for deletion, let's loop over them.
            ed::LinkId linkId;
            while (ed::QueryDeletedLink(&linkId))
            {
                // If you agree that link can be deleted, accept deletion.
                if (ed::AcceptDeletedItem())
                {
                    graph.removeLink(linkId);
                }

                // You may reject link deletion by calling:
                // ed::RejectDeletedItem();
            }

            ed::NodeId nodeId;
            while (ed::QueryDeletedNode(&nodeId))
            {
                graph.removeNode(nodeId);
            }
        }
        ed::EndDelete(); // Wrap up deletion action
    }

    void createNewNodePopup()
    {
        ed::Suspend();
        ImGui::SetNextWindowSize({300.f, 400.f});
        if (ImGui::BeginPopup("Create New Node"))
        {
            ImGui::Dummy({200.f, 0.f});

            auto newNodePostion = ed::ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());
            NodeId newNode{0};

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 3));

            static std::string filterStr;
            static int selectionIndex = -1;

            if (ImGui::IsWindowAppearing())
            {
                filterStr.clear();
                ImGui::SetKeyboardFocusHere(0);
                ImGui::SetScrollY(0);
            }

            bool openAll = ImGui::InputTextWithHint("##filter", ICON_FA_MAGNIFYING_GLASS " Search", &filterStr);

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.25f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

            ImGui::Checkbox("##filter_bool", &newNodeFilterType);

            ImGui::PopStyleColor();
            ImGui::PopStyleVar();

            ImGui::SetItemTooltip("Filter by node type");

            if (openAll || ImGui::IsWindowAppearing())
            {
                selectionIndex = -1;
            }

            bool selectionChanged = false;

            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            {
                openAll = true;
                selectionIndex++;
                selectionChanged = true;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            {
                openAll = true;
                selectionIndex--;
                selectionChanged = true;
            }

            const bool createAtIndex = ImGui::IsKeyPressed(ImGuiKey_Enter);

            std::unordered_map<std::string, std::vector<std::string>> categories;

            const auto isNameFilteredOut = [&](const auto& arch)
            {
                if (filterStr.empty())
                {
                    return false;
                }

                if (arch.category.contains(filterStr))
                {
                    return false;
                }

                if (arch.id.contains(filterStr))
                {
                    return false;
                }

                if (arch.title.contains(filterStr))
                {
                    return false;
                }

                return true;
            };

            const auto isTypeFilteredOut = [&](const auto& arch)
            {
                if (!newNodeFilterType)
                {
                    return false;
                }

                if (newNodeTargetPins.empty())
                {
                    return false;
                }

                for (const auto targetPin : newNodeTargetPins)
                {
                    if (targetPin.direction() == PinDirection::In)
                    {
                        if (findConnectTarget(arch, targetPin) >= 0)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (findConnectTarget(targetPin, arch) >= 0)
                        {
                            return false;
                        }
                    }
                }

                return true;
            };

            for (const auto& [id, archetype] : archetypes.archetypes)
            {
                if (!isNameFilteredOut(archetype) && !isTypeFilteredOut(archetype))
                {
                    categories[archetype.category].push_back(archetype.id);
                }
            }

            int currentIndex = 0;

            for (auto& [category, archetypeIds] : categories)
            {
                if (category.empty())
                {
                    continue;
                }

                std::ranges::sort(archetypeIds);

                if (openAll)
                {
                    ImGui::SetNextItemOpen(true);
                }

                ImGui::BeginChild("options");
                if (ImGui::TreeNodeEx(category.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, category.c_str()))
                {
                    for (const auto& archetypeId : archetypeIds)
                    {
                        const auto& archetype = archetypes.archetypes[archetypeId];
                        const auto isSelected = selectionIndex == currentIndex;

                        if (isSelected)
                        {
                            ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
                        }

                        if (ImGui::Selectable(archetype.title.c_str(), isSelected) || (createAtIndex && isSelected))
                        {
                            newNode = graph.AddNode(archetype.createNode()).id;
                        }

                        if (isSelected && selectionChanged)
                        {
                            ImGui::ScrollToRect(ImGui::GetCurrentWindow(),
                                                {ImGui::GetItemRectMin(), ImGui::GetItemRectMax()});
                        }

                        if (isSelected)
                        {
                            ImGui::PopStyleColor(1);
                        }

                        currentIndex++;
                    }

                    ImGui::TreePop();
                }
                ImGui::EndChild();
            }

            ImGui::PopStyleVar();

            if (currentIndex > 0)
            {
                selectionIndex = std::clamp(selectionIndex, 0, currentIndex - 1);
            }

            if (newNode)
            {
                ImGui::CloseCurrentPopup();

                ed::SetNodePosition(newNode, newNodePostion);

                const auto& arch = *graph.findNode<ExpressionNode>(newNode)->archetype;

                for (const auto targetPin : newNodeTargetPins)
                {
                    if (targetPin.direction() == PinDirection::In)
                    {
                        if (auto index = findConnectTarget(arch, targetPin); index >= 0)
                        {
                            graph.addLink(targetPin, newNode.makeOutput(index));
                        }
                    }
                    else
                    {
                        if (auto index = findConnectTarget(targetPin, arch); index >= 0)
                        {
                            graph.addLink(targetPin, newNode.makeInput(index));
                        }
                    }
                }

                newNodeTargetPins.clear();
            }

            ImGui::EndPopup();
        }
        else
        {
            newNodeTargetPins.clear();
        }
        ed::Resume();
    }

    void serialize(Serializer& s)
    {
        if (!s.isSaving)
        {
            graph.nodes.clear();
            graph.links.clear();
        }

        NodeSerializer::repo = &archetypes;
        s.serialize("nodes", graph.nodes);
        s.serialize("links", graph.links);

        s.serializeValue<float>("zoom", &ed::GetViewZoom, &ed::SetViewZoom);
        s.serializeValue<ImVec2>("scroll", &ed::GetViewScroll, &ed::SetViewScroll);

        if (!s.isSaving)
        {
            ShortId maxId{};
            for (const auto& node : graph.nodes)
            {
                if (!node)
                {
                    continue;
                }

                maxId = std::max(maxId, node->id.Get());
            }
            graph.idPool.reset(maxId + 1);
        }
    }

    std::string copySelectedToString()
    {
        auto nodeIds = GraphUtils::getSelectedNodes();
        if (nodeIds.size() == 0)
        {
            return {};
        }

        std::erase_if(nodeIds, [&](auto node) { return graph.findNode<OutputNode>(node) != nullptr; });

        auto links = graph.links;
        std::erase_if(links,
                      [&](auto link)
                      { return !nodeIds.contains(link.from().nodeId()) || !nodeIds.contains(link.to().nodeId()); });

        auto nodesToSave = nodeIds |
                           std::views::transform([&](auto node) { return graph.findNode<ExpressionNode>(node); }) |
                           std::ranges::to<std::vector>();

        json j;
        Serializer s(true, j);

        NodeSerializer::repo = &archetypes;

        s.serialize("nodes", nodesToSave);
        s.serialize("links", links);

        j["type"] = "MLS-subgraph";

        return j.dump(4);
    }

    void pasteFromString(std::string_view src)
    {
        try
        {
            json j = json::parse(src);
            if (j["type"] != "MLS-subgraph")
            {
                return;
            }

            std::unordered_map<NodeId, NodeId> newIdMap;
            for (auto& node : j["nodes"])
            {
                const auto oldId = node["id"].template get<NodeId::InnerType>();
                const auto newId = graph.idPool.take();
                newIdMap.emplace(oldId, newId);
                node["id"] = newId;
            }

            Serializer s(false, j);
            NodeSerializer::repo = &archetypes;

            std::vector<LinkId> links;
            std::vector<Graph::Node::Ptr> nodes;

            s.serialize("nodes", nodes);
            s.serialize("links", links);

            sf::Vector2f boundsMin;
            sf::Vector2f boundsMax;

            for (const auto& node : nodes)
            {
                const auto pos = ed::GetNodePosition(node->id);

                boundsMin.x = std::min(boundsMin.x, pos.x);
                boundsMin.y = std::min(boundsMin.y, pos.y);

                boundsMax.x = std::max(boundsMax.x, pos.x);
                boundsMax.y = std::max(boundsMax.y, pos.y);
            }

            const auto viewRect = reinterpret_cast<ax::NodeEditor::Detail::EditorContext*>(ed::GetCurrentEditor())->GetViewRect();
            const auto center = (viewRect.Min + viewRect.Max) / 2;

            const auto offset = center - (boundsMin + boundsMax) / 2.f;

            ed::ClearSelection();
            for (auto& node : nodes)
            {
                const auto id = node->id;
                ed::SelectNode(id, true);
                ed::SetNodePosition(id, ed::GetNodePosition(id) + offset);
                graph.AddNode(std::move(node));
            }

            for (auto& link : links)
            {
                PinId from = link.from();
                PinId to = link.to();

                to = PinId::makeInput(newIdMap.at(to.nodeId()), to.index());
                from = PinId::makeOutput(newIdMap.at(from.nodeId()), from.index());

                graph.addLink(from, to);
            }
        } catch (...)
        {
        }
    }

    void deleteSelected()
    {
        for (const auto nodeId : GraphUtils::getSelectedNodes())
        {
            graph.removeNode(nodeId);
        }
    }

    void ensureOutputNodesExist()
    {
        bool vertexOutFound = false;
        bool fragmentOutFound = false;

        for (auto& node : graph.nodes)
        {
            if (!node)
            {
                continue;
            }

            if (auto* n = dynamic_cast<OutputNode*>(node.get()))
            {
                if (n->type == CodeGenerator::Type::Vertex)
                {
                    vertexOutFound = true;
                }
                else if (n->type == CodeGenerator::Type::Fragment)
                {
                    fragmentOutFound = true;
                }
            }
        }

        if (!vertexOutFound)
        {
            auto node = graph.AddNode(archetypes.get("out_vertex").createNode());
            ed::SetNodePosition(node.id,
                                ed::GetViewScroll() / ed::GetViewZoom() + ed::GetViewSize() * ImVec2(1.f, 0.5f) +
                                    ImVec2(-200.f, -100.f));
        }

        if (!fragmentOutFound)
        {
            auto node = graph.AddNode(archetypes.get("out_fragment").createNode());
            ed::SetNodePosition(node.id,
                                ed::GetViewScroll() / ed::GetViewZoom() + ed::GetViewSize() * ImVec2(1.f, 0.5f) +
                                    ImVec2(-200.f, 100.f));
        }
    }

    void draw()
    {
        const auto mousePos = ImGui::GetMousePos();

        ImGui::BeginChild("graph", {}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::Separator();
        ed::Begin("graph-editor", ImVec2(0.0, 0.0f));

        for (auto& node : graph.nodes)
        {
            if (!node)
            {
                continue;
            }

            static_cast<ExpressionNode&>(*node).draw();
        }

        if (const LinkId link = ed::GetDoubleClickedLink())
        {
            auto& node = static_cast<ExpressionNode&>(graph.AddNode(archetypes.archetypes["bridge"].createNode()));
            ed::SetNodePosition(node.id, ed::ScreenToCanvas(mousePos));

            graph.addLink(link.from(), node.id.makeInput(0));
            graph.addLink(node.id.makeOutput(0), link.to());

            graph.removeLink(link);
        }

        for (auto& link : graph.links)
        {
            const auto from = link.from();
            const auto to = link.to();
            const auto& node = graph.getNode<ExpressionNode>(to.nodeId());
            const auto& input = node.inputs[to.index()];
            const auto color = input.hasError() ? LinkColorError : LinkColor;

            ed::Link(link, from, to, color, 2.f);
        }

        std::string error{};
        if (const LinkId link = ed::GetHoveredLink())
        {
            const auto to = link.to();
            const auto& node = graph.getNode<ExpressionNode>(to.nodeId());
            const auto& input = node.inputs[to.index()];
            if (input.hasError())
            {
                error = input.error;
            }
        }
        else if (const PinId pin = ed::GetHoveredPin(); pin && pin.direction() == PinDirection::In)
        {
            const auto& node = graph.getNode<ExpressionNode>(pin.nodeId());
            const auto& input = node.inputs[pin.index()];
            if (input.hasError())
            {
                error = input.error;
            }
        }

        if (!error.empty())
        {
            ed::Suspend();
            if (ImGui::BeginTooltip())
            {
                ImGui::Text(error.c_str());
                ImGui::EndTooltip();
            }
            ed::Resume();
        }

        ed::Suspend();
        if (ed::ShowBackgroundContextMenu())
        {
            ImGui::OpenPopup("Create New Node");
        }
        ed::Resume();

        createNewNodePopup();

        if (ed::IsBackgroundDoubleClicked())
        {
            ed::NavigateToContent(0.25f);
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyAlt)
        {
            if (const PinId pin = ed::GetHoveredPin())
            {
                graph.removeLinks(pin);
            }
        }

        updateCreate();
        updateDelete();

        ed::End();

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ParameterDrag"))
            {
                std::string parameterId((const char*)payload->Data, payload->DataSize);

                auto& node = static_cast<ParameterNode&>(graph.AddNode(archetypes.archetypes["parameter"].createNode()));
                ed::SetNodePosition(node.id, ed::ScreenToCanvas(mousePos));

                node.parameterId = parameterId;
            }
        }

        ImGui::Separator();
        ImGui::EndChild();

        ensureOutputNodesExist();
    }

    void update()
    {
        for (auto& node : graph.nodes)
        {
            if (!node)
            {
                continue;
            }

            static_cast<ExpressionNode&>(*node).update(&graphContext);
        }

        for (auto& link : graph.links)
        {
            graph.getNode<ExpressionNode>(link.to().nodeId()).inputs[link.to().index()].link = link;
            graph.getNode<ExpressionNode>(link.from().nodeId()).outputs[link.from().index()].linkCount++;
        }
    }
};

inline void serialize(Serializer& s, GraphEditor& editor)
{
    editor.serialize(s);
}