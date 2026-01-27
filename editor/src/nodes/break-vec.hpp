#pragma once

#include <array>

#include "archetypes.hpp"
#include "expression.hpp"

struct BreakVecNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	uint8_t arrity;

	BreakVecNode(NodeArchetype* archetype, uint8_t arrity) : ExpressionNode{ archetype }, arrity{ arrity }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
		auto value = getInput(0);

		for (uint8_t x = 0; x < arrity; x++)
		{
			if (value)
			{
				setOutput(x, {Types::scalar, value.code + "." + ("xyza"[x])});
			}
			else
			{
				setOutput(x, Values::zero);
			}
		}
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		for (uint8_t arrity = 2; arrity <= 4; arrity++)
		{
			std::vector<NodeArchetype::Output> outputs;

			for (uint8_t x = 0; x < arrity; x++)
			{
				outputs.push_back({ std::string(1, "XYZA"[x]), Types::scalar });
			}

			repo.add<BreakVecNode>({
				"Value",
				"break_vec" + std::to_string(arrity),
				"Break Vec" + std::to_string(arrity),
				{
					{ .name="", .type=Types::makeVec(arrity), .showField=false}
				},
				outputs
			}, arrity);
		}
	}
};