#pragma once

#include <array>

struct BuiltinFuncNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	std::string func;
	std::vector<FloatField> arguments;

	BuiltinFuncNode(NodeArchetype* archetype, std::string func) : ExpressionNode{ archetype }, func{ func }
	{

	}

	void update(const Graph& graph) override
	{
		ExpressionNode::update(graph);

		if (arguments.size() != inputs.size())
		{
			arguments.resize(inputs.size());
		}
	}

	void evaluate(CodeGenerator& generator) override
	{
		std::string code = func + "(";

		const auto inputCount = inputs.size();
		for (std::size_t x = 0; x < inputCount; x++)
		{
			auto value = getInput(x);
			if (!value)
			{
				value = arguments[x].toValue();
			}

			code += value.code;

			if (x < inputCount - 1)
			{
				code += ", ";
			}
		}

		code += ")";

		setOutput(0, Value{ outputs[0].type, code});
	}

	void drawInputPin(Input& input, PinId id) override
	{
		ExpressionNode::drawInputPin(input, id);

		if (!input.link)
		{
			ImGui::SameLine();

			arguments[id.index()].update();
		}
	};

	void serialize(Serializer& s) override
	{
		ExpressionNode::serialize(s);

		s.serialize("arguments", arguments);
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<BuiltinFuncNode>({
			"Maths",
			"lerp",
			"Lerp",
			{
				{ "X", makeValueType<ScalarType>()},
				{ "Y", makeValueType<ScalarType>()},
				{ "A", makeValueType<ScalarType>()},
			},
			{
				{ "", makeValueType<ScalarType>()},
			}
		}, "mix");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"mod",
			"Modulo",
			{
				{ "X", makeValueType<ScalarType>()},
				{ "Y", makeValueType<ScalarType>()},
			},
			{
				{ "", makeValueType<ScalarType>()},
			}
		}, "mod");

		repo.add<BuiltinFuncNode>({
			"Maths",
			"abs",
			"Absolute",
			{
				{ "", makeValueType<ScalarType>()},
			},
			{
				{ "", makeValueType<ScalarType>()},
			}
		}, "abs");
	}
};
