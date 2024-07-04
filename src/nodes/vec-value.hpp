#pragma once

#include <array>

#include "archetypes.hpp"
#include "expression.hpp"

struct VecValueNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	uint8_t arrity;
	std::array<FloatField, 4> floatFields;

	VecValueNode(NodeArchetype* archetype, uint8_t arrity) : ExpressionNode{ archetype }, arrity{ arrity }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
		Value value;

		value.code = std::format("vec{}(", arrity);
		for (uint8_t x = 0; x < arrity; x++)
		{
			if (x != 0)
			{
				value.code += ", ";
			}

			value.code += floatFields[x].toValue().code;
		}

		value.code += ")";
		value.type = makeValueType<VectorType>(arrity);

		setOutput(0, value);
	}

	void drawInputPins() override
	{
		for (uint8_t x = 0; x < arrity; x++)
		{
			ImGui::PushItemWidth(15);
			ImGui::LabelText("", "%c", "XYZA"[x]);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			floatFields[x].update();

			if (x == 0 && (arrity == 3 || arrity == 4))
			{
				std::array<float, 4> floats;
				for (uint8_t y = 0; y < arrity; y++)
				{
					floats[y] = floatFields[y].value;
				}

				ImGui::SameLine();

				const auto flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | (arrity == 3 ? ImGuiColorEditFlags_NoAlpha : 0);
				const  bool colorChanged = ImGui::ColorEdit4("##colorEdit", floats.data(), flags);

				for (uint8_t y = 0; y < arrity; y++)
				{
					floatFields[y].value = floats[y];

					if (colorChanged)
					{
						floatFields[y].widthDirty = true;
					}
				}
			}
		}
	}

	void serialize(Serializer& s) override
	{
		ExpressionNode::serialize(s);

		s.serialize("fields", floatFields);
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		for (uint8_t arrity = 2; arrity <= 4; arrity++)
		{
			repo.add<VecValueNode>({
				"Constants",
				"vec" + std::to_string(arrity),
				"Vec" + std::to_string(arrity),
				{},
				{
					{ "", makeValueType<VectorType>(arrity)}
				}
			}, arrity);
		}
	}
};