#pragma once

#include "ClipRectScopeGuard.hpp"
#include "archetypes.hpp"
#include "code-generator.hpp"
#include "expression.hpp"
#include "mls/material.hpp"

#include <array>
#include <format>
#include <iostream>
#include <print>
#include <vector>

#include <cstdint>

struct CodeNode : ExpressionNode
{
    using ExpressionNode::ExpressionNode;

    CodeGenerator::Function function;

    std::string codeEditorString;
    TextEditor codeEditor;

    CodeNode(struct NodeArchetype* archetype) : ExpressionNode(archetype)
    {
        auto lang = TextEditor::LanguageDefinition::GLSL();
        codeEditor.SetLanguageDefinition(lang);
        codeEditor.SetPalette(TextEditor::GetDarkPalette());
    }

    void update(GraphContext* inGraphContext) override
    {
        ExpressionNode::update(inGraphContext);

        function.params.resize(inputs.size() + outputs.size());

        for (int x = 0; x < outputs.size(); x++)
        {
            const auto index = outputs.size() - x - 1;
            auto& output = outputs[index];

            if (output.toRemove)
            {
                function.params.erase(function.params.begin() + index + inputs.size());
            }
            else
            {
                auto& param = function.params[index + inputs.size()];
                param.isOut = true;
                param.id = std::format("out_{}", index);
                output.type = param.type;
            }
        }

        for (int x = 0; x < inputs.size(); x++)
        {
            const auto index = inputs.size() - x - 1;

            auto& input = inputs[index];

            if (input.toRemove)
            {
                function.params.erase(function.params.begin() + index);
            }
            else
            {
                auto& param = function.params[index];
                param.isOut = false;
                param.id = std::format("in_{}", index);
             
                input.type = param.type;
                input.requireLink = true;
            }
        }

        function.id = std::format("custom_func_{}", id.Get());
        function.returnType = Types::none;
    }

    void evaluate(CodeGenerator& generator) override
    {
        std::vector<Value> args;
        for (int x = 0; x < inputs.size(); x++)
        {
            const auto input = getInput(x);
            if (!input)
            {
                return;
            }

            args.push_back(input);
        }

        for (int x = 0; x < outputs.size(); x++)
        {
            auto var = generator.addEmptyVar(outputs[x].type);
            args.push_back(var);
            setOutput(x, std::move(var));
        }
        
        generator.addFunc(function);
        generator.callFunc(function.id, args);
    }

    void showParameterTypeCombo(CodeGenerator::Param& param)
    {
        static const std::array<ValueType, 5> types{
            Types::scalar,
            Types::vec2,
            Types::vec3,
            Types::vec4,
            Types::texture,
        };

        ClipRectScopeGuard clipRectGuard;
        if (ImGui::BeginCombo("##type", param.type.getName().c_str(), ImGuiComboFlags_WidthFitPreview))
        {
            for (const auto& type : types)
            {
                bool isSelected = type == param.type;
                if (ImGui::Selectable(type.getName().c_str(), isSelected))
                {
                    param.type = type;
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    virtual void drawInputPin(Input& input, PinId id)
    {
        ExpressionNode::drawInputPin(input, id);
    }

    virtual void drawInputPins()
    {
        ViewportScopeGuard viewportGuard;

        for (PinId::PinIndex x{}; x < inputs.size() && x < function.params.size(); x++)
        {
            auto& input = inputs[x];
            input.showField = false;
            const auto pinId = id.makeInput(x);
            ed::BeginPin(pinId, ed::PinKind::Input);
            drawInputPin(input, pinId);
            ed::EndPin();
            ImGui::SameLine();

            ImGui::PushID(x);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.f);
            if (ImGui::Button(ICON_FA_CIRCLE_MINUS))
            {
                input.toRemove = true;
            }
            ImGui::PopStyleVar();

            ImGui::SameLine();

            auto& param = function.params[x];

            showParameterTypeCombo(param);

            ImGui::PopID();
        }

        ImGui::Dummy({13.f, 0.f});
        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.f);
        if (ImGui::Button(ICON_FA_CIRCLE_PLUS))
        {
            inputs.emplace_back();
        }
        ImGui::PopStyleVar();
    }

    virtual void drawOutputPins()
    {
        ViewportScopeGuard viewportGuard;

        for (PinId::PinIndex x{}; x < outputs.size() && (inputs.size() + x) < function.params.size(); x++)
        {
            ImGui::PushID(x);

            ImGui::BeginHorizontal("superGroup");

            ImGui::Spring();

            ImGui::BeginHorizontal("firstGroup");

            auto& output = outputs[x];
            const auto pinId = id.makeOutput(x);

            auto& param = function.params[x + inputs.size()];

            showParameterTypeCombo(param);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.f);
            if (ImGui::Button(ICON_FA_CIRCLE_MINUS))
            {
                output.toRemove = true;
            }
            ImGui::PopStyleVar();

            ImGui::EndHorizontal();

            ImGui::BeginVertical("secondGroup");

            ed::BeginPin(pinId, ed::PinKind::Output);
            drawOutputPin(output, pinId);
            ed::EndPin();

            ImGui::EndVertical();

            ImGui::EndHorizontal();

            ImGui::PopID();
        }

        ImGui::BeginHorizontal("h");
        ImGui::Spring(1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.f);
        if (ImGui::Button(ICON_FA_CIRCLE_PLUS))
        {
            outputs.emplace_back();
        }
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::Dummy({37.f, 1.f});
        ImGui::EndHorizontal();
    }

    void drawMiddle() override
    {
        ClipRectScopeGuard clipRectGuard;

        auto makeTooltip = [&](size_t maxLineLength, size_t maxLineCount) -> std::string
        {
            const auto cutStringAfterMaxLength = [&](const std::string& text) -> std::string
            {
                if (text.size() <= maxLineLength)
                {
                    return text;
                }
                std::string r = text.substr(0, maxLineLength - 3) + "...";
                return r;
            };

            std::string r = "";
            size_t n = 0;
            for (const auto& line : function.body)
            {
                if (n >= maxLineCount)
                {
                    r += "...\n";
                    break;
                }
                r += cutStringAfterMaxLength(line) + "\n";
                n++;
            }
            return r;
        };

        // Helper function to make the text color more red
        auto fn_redify_color = [](const ImVec4& color) -> ImVec4
        {
            ImVec4 redified_color;
            redified_color.x = color.x;
            redified_color.y = 0.6f * color.y;
            redified_color.z = 0.6f * color.z;
            redified_color.w = color.w;
            return redified_color;
        };

        //ImGui::BeginVertical("EditVer");

        ImGui::Spring();


        ImGui::Dummy({20, 0});
        ImGui::SameLine();

        bool was_button_pressed = false;
        if (ImGui::Button("Edit Code"))
        {
            was_button_pressed = true;
            ImGui::OpenPopup("InputTextMultilinePopup");
        }
        else
        {
            ImGui::SetItemTooltip("%s", makeTooltip(60, 3).c_str());
        }

        ImGui::SameLine();
        ImGui::Dummy({20, 0});

        if (ImGui::BeginPopup("InputTextMultilinePopup"))
        {
            std::string signature = std::format("{} {{", function.makeSignature());
            ImGui::Text("%s", signature.c_str());
            codeEditor.Render("codeEditor", {0, 400});
            ImGui::Text("}");

            ImGui::EndPopup();
        }

        if (codeEditor.IsTextChanged())
        {
            function.body = codeEditor.GetTextLines();
        }
    }

    void serialize(Serializer& s) override
    {
        ExpressionNode::serialize(s);

        s.serialize("functionDef", function);

        if (!s.isSaving)
        {
            codeEditor.SetTextLines(function.body);
        }
    }

    static void registerArchetypes(ArchetypeRepo& repo)
    {
        repo.add<CodeNode>({
            "Utils",
            "code",
            "Code",
            {
                {"", Types::scalar},
                {"", Types::scalar},
                {"", Types::scalar},
                {"", Types::scalar},
            },
            {
                {"", Types::scalar},
            },
        });
    }
};