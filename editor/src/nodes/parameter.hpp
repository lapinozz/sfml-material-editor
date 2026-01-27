#pragma once

#include <array>
#include <format>

#include "archetypes.hpp"
#include "expression.hpp"

#include "ViewportScopeGuard.hpp"

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
			const auto type = it->second;
			outputs[0].type = type;

			const std::string parameterName = std::format("P_{}", parameterId);

			setOutput(0, Value{it->second, parameterName });

			generator.shaderInputs[parameterName] = type;
		}
	}

	void drawInputPins() override
	{
		ViewportScopeGuard viewportGuard;

		if (ImGui::BeginCombo("##parameter", parameterId.c_str(), ImGuiComboFlags_WidthFitPreview | ImGuiComboFlags_PopupAlignLeft))
		{
			if (ImGui::IsWindowAppearing())
			{
				ImGui::SetWindowPos(ImGui::GetWindowPos() + ImVec2(200.f, 0.f));

			}
			//ImGui::SetWindowPos(,);
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

	void serialize(Serializer& s) override
	{
		ExpressionNode::serialize(s);

		s.serialize("parameterId", parameterId);
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<ParameterNode>({
			"Inputs",
			"parameter",
			"Parameter",
			{},
			{
				{ "", Types::scalar}
			}
		});
	}
};