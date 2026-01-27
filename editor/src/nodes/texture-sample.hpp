#pragma once

#include <array>

#include "archetypes.hpp"
#include "expression.hpp"

struct SampleTextureNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	SampleTextureNode(NodeArchetype* archetype) : ExpressionNode{ archetype }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
		Value value;
		value.code = std::format("texture2D({}, {})", getInput(0).code, getInput(1).code);
		value.type = Types::vec4;

		const Value result = generator.addVar(value);

		setOutput(0, result);
		setOutput(1, { Types::vec3, result.code + ".rgb"});
		setOutput(2, { Types::scalar, result.code + ".a" });
	}

	void drawMiddle() override
	{
		ImGui::Dummy({ 8.f, 0.f });
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<SampleTextureNode>({
			"Texture",
			"sample_texture",
			"Sample Texture",
			{
				{"Texture", makeValueType<SamplerType>()},
				{"UV", Types::vec2},
			},
			{
				{ "RGBA", Types::vec4},
				{ "RGB", Types::vec3 },
				{ "A", Types::scalar}
			}
		});
	}
};