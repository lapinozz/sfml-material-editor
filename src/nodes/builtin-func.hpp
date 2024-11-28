#pragma once

#include <array>

struct BuiltinFuncNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	std::string func;

	BuiltinFuncNode(NodeArchetype* archetype, std::string func) : ExpressionNode{ archetype }, func{ func }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
		std::string code = func + "(";

		const auto inputCount = inputs.size();
		for (std::size_t x = 0; x < inputCount; x++)
		{
			code += getInput(x).code;

			if (x < inputCount - 1)
			{
				code += ", ";
			}
		}

		code += ")";

		setOutput(0, Value{ outputs[0].type, code});
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<BuiltinFuncNode>({
			"Maths",
			"lerp",
			"Lerp",
			{
				{ "X", Types::scalar},
				{ "Y", Types::scalar},
				{ "A", Types::scalar},
			},
			{
				{ "", Types::scalar},
			}
		}, "mix");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"mod",
			"Modulo",
			{
				{ "X", Types::scalar},
				{ "Y", Types::scalar},
			},
			{
				{ "", Types::scalar},
			},
			{{{Types::none, Types::none}, {Types::none}}},
		}, "mod");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"abs",
			"Absolute",
			{
				{ "", Types::scalar},
			},
			{
				{ "", Types::scalar},
			}
		}, "abs");
	}
};
