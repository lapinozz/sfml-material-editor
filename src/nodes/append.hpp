#pragma once

#include <array>

struct AppendNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	void evaluate(CodeGenerator& generator) override
	{
		auto& ia = inputs.at(0);
		auto& ib = inputs.at(1);

		const auto& a = getInput(0);
		const auto& b = getInput(1);

		if (!a || !b)
		{
			return;
		}

		int arrityA{};
		int arrityB{};

		if (const auto* t = std::get_if<GenType>(&a.type))
		{
			arrityA = t->arrity;
		}
		else
		{
			ia.error = "Value is not a GenType";
			return;
		}

		if (const auto* t = std::get_if<GenType>(&b.type))
		{
			arrityB = t->arrity;
		}
		else
		{
			ib.error = "Value is not a GenType";
			return;
		}

		const std::uint8_t totalArrity = arrityA + arrityB;
		if(totalArrity > 4)
		{
			ia.error = "Inputs cannot have more than 4 components in total";
			ib.error = ia.error;
			return;
		}

		const auto resultType = Types::makeVec(totalArrity);
		outputs.at(0).type = resultType;

		setOutput(0, Value{ resultType, resultType.toString() + "(" + a.code + ", " + b.code + ")"});
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<AppendNode>({
			"Value",
			"append",
			"Append",
			{
				{ .name = "A", .type = makeValueType<NoneType>(), .requireLink = true},
				{ .name = "B", .type = makeValueType<NoneType>(), .requireLink = true}
			},
			{
				{ "", makeValueType<NoneType>()},
			},
		});
	}
};
