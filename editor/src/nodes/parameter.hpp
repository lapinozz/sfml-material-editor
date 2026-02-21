#pragma once

#include "ViewportScopeGuard.hpp"
#include "archetypes.hpp"
#include "expression.hpp"
#include "mls/material.hpp"

#include <array>
#include <format>

struct ParameterNode : ExpressionNode
{
    using ExpressionNode::ExpressionNode;

    std::string parameterId;

    const ParameterTypeMap* parameterMap;

    void update(GraphContext* inGraphContext) override
    {
        ExpressionNode::update(inGraphContext);

        const auto it = graphContext->parameterTypes.find(parameterId);
        if (it != graphContext->parameterTypes.end())
        {
            outputs[0].type = it->second;
            
            if (it->second == Types::texture)
            {
                outputs.resize(2);

                outputs[0].name = "texture";
                outputs[1].name = "size";

                outputs[1].type = Types::vec2;
            }
            else if (outputs.size() == 2)
            {
                outputs[1].toRemove = true;
                outputs[0].name = "";
            }
        }
    }

    void evaluate(CodeGenerator& generator) override
    {
        const auto it = graphContext->parameterTypes.find(parameterId);
        if (it == graphContext->parameterTypes.end())
        {
            error = std::format("Could not find parameter \"{}\"", parameterId);
        }
        else
        {
            const auto addOutput = [&](auto index, auto type, auto name) 
            {
                outputs[index].type = type;

                const std::string parameterName = std::format("{}{}", Material::uniformPrefix, name);

                setOutput(index, Value{type, parameterName});

                generator.shaderInputs[parameterName] = type;
            };

            addOutput(0, it->second, parameterId);

            if (it->second == Types::texture)
            {
                addOutput(1, Types::vec2, std::format("{}{}", parameterId, Material::textureUniformSizeSuffix));
            }
        }
    }

    void drawInputPins() override
    {
        ViewportScopeGuard viewportGuard;

        if (ImGui::BeginCombo("##parameter", parameterId.c_str(), ImGuiComboFlags_WidthFitPreview | ImGuiComboFlags_PopupAlignLeft))
        {
            for (const auto& pair : graphContext->parameterTypes)
            {
                const auto& param = pair.first;
                const bool is_selected = parameterId == param;
                if (ImGui::Selectable(param.c_str(), is_selected))
                {
                    parameterId = param;
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    void drawMiddle() override
    {
        ImGui::Dummy({8.f, 0.f});
    }

    void serialize(Serializer& s) override
    {
        ExpressionNode::serialize(s);

        s.serialize("parameterId", parameterId);
    }

    static void registerArchetypes(ArchetypeRepo& repo)
    {
        repo.add<ParameterNode>({"Inputs", "parameter", "Parameter", {}, {{"", Types::scalar}}});
    }
};