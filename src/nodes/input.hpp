#pragma once

#include "../value.hpp"

struct InputNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	ValueType type;
	std::string input;

	InputNode(NodeArchetype* archetype, ValueType type, std::string input) : ExpressionNode{ archetype }, type{ type }, input { std::move(input) }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
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
		}, Types::scalar, "time");

		repo.add<InputNode>({
			"Inputs",
			"uv",
			"UV",
			{
			},
			{
				{ "", Types::scalar}
			}
		}, Types::makeVec(uint8_t{2}), "gl_TexCoord[0].xy");
	}
};