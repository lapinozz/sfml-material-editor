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
			},
			{
				{ {Types::none, Types::none, Types::scalar}, {Types::none} },
				{ {Types::none, Types::none, Types::none}, {Types::none} }
			},
		}, "mix");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"clamp",
			"Clamp",
			{
				{ "Value", Types::scalar},
				{ "Min", Types::scalar},
				{ "Max", Types::scalar},
			},
			{
				{ "", Types::scalar},
			},
			{
				{ {Types::none, Types::none, Types::none}, {Types::none} },
				{ {Types::none, Types::scalar, Types::scalar}, {Types::none} }
			},
		}, "clamp");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"mod",
			"Modulo",
			{
				{ "A", Types::scalar},
				{ "B", Types::scalar},
			},
			{
				{ "", Types::scalar},
			},
			{
				{ {Types::none, Types::scalar}, {Types::none} },
				{ {Types::none, Types::none}, {Types::none} }
			},
		}, "mod");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"min",
			"Min",
			{
				{ "A", Types::scalar},
				{ "B", Types::scalar},
			},
			{
				{ "", Types::scalar},
			},
			{
				{ {Types::none, Types::scalar}, {Types::none} },
				{ {Types::none, Types::none}, {Types::none} }
			},
		}, "min");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"max",
			"Max",
			{
				{ "A", Types::scalar},
				{ "B", Types::scalar},
			},
			{
				{ "", Types::scalar},
			},
			{
				{ {Types::none, Types::scalar}, {Types::none} },
				{ {Types::none, Types::none}, {Types::none} }
			},
		}, "max");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"pow",
			"Pow",
			{
				{ "A", Types::scalar},
				{ "B", Types::scalar},
			},
			{
				{ "", Types::scalar},
			},
			{
				{ {Types::none, Types::none}, {Types::none} }
			},
		}, "pow");

		auto addSingleInputOutputGen = [&](auto func, auto name)
		{
			repo.add<BuiltinFuncNode>({
				"Maths",
				func,
				name,
				{
					{ "", Types::scalar},
				},
				{
					{ "", Types::scalar},
				},
				{
					{ {Types::none}, {Types::none} }
				}
				}, func);
		};

		repo.add<BuiltinFuncNode>({
			"Maths",
			"length",
			"Length",
			{
				{ "", Types::scalar},
			},
			{
				{ "", Types::scalar},
			},
			{
				{ {Types::none}, {Types::scalar} }
			}
		}, "length");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"distance",
			"Distance",
			{
				{ "P0", Types::scalar},
			},
			{
				{ "P1", Types::scalar},
			},
			{
				{ {Types::none, Types::none}, {Types::scalar} }
			}
		}, "distance");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"dot",
			"Dot",
			{
				{ "P0", Types::scalar},
			},
			{
				{ "P1", Types::scalar},
			},
			{
				{ {Types::none, Types::none}, {Types::scalar} }
			}
		}, "dot");

		addSingleInputOutputGen("abs", "Absolute");
		addSingleInputOutputGen("sign", "Sign");
		addSingleInputOutputGen("floor", "Floor");
		addSingleInputOutputGen("ceil", "Ceil");
		addSingleInputOutputGen("fract", "Fractional");
		addSingleInputOutputGen("trunc", "Truncate");
		addSingleInputOutputGen("sqrt", "Sqrt");
		addSingleInputOutputGen("exp", "Exp");
		addSingleInputOutputGen("log", "Log");
		addSingleInputOutputGen("log2", "Log2");
		addSingleInputOutputGen("sin", "Sin");
		addSingleInputOutputGen("cos", "Cos");
		addSingleInputOutputGen("tan", "Tan");
		addSingleInputOutputGen("asin", "ASin");
		addSingleInputOutputGen("acos", "ACos");
		addSingleInputOutputGen("atan", "ATan");
		addSingleInputOutputGen("normalize", "Normalize");

	}
};
