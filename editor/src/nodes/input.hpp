#pragma once

#include "../value.hpp"

struct InputNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	ValueType type;
	std::string input;
	bool isUniform{};

	InputNode(NodeArchetype* archetype, ValueType type, std::string input, bool isUniform) : 
		ExpressionNode{ archetype }, 
		type{ type }, 
		input { std::move(input) }, 
		isUniform{ isUniform }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
		if (isUniform)
		{
			generator.shaderInputs[input] = type;
		}

		setOutput(0, Value{ type, input });
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<InputNode>({
			"Inputs",
			"input_time",
			"Time",
			{
			},
			{
				{ "", Types::scalar}
			}
		}, Types::scalar, "P_time", true);

		repo.add<InputNode>({
			"Inputs",
			"uv",
			"UV",
			{
			},
			{
				{ "", Types::vec2}
			}
		}, Types::vec2, "gl_TexCoord[0].xy", false);
	}
};