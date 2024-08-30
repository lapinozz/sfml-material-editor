#pragma once

#include <array>

#include "../value.hpp"

struct ConstantNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	ValueType type;
	std::array<FloatField, 4> floatFields;

	ConstantNode(NodeArchetype* archetype, ValueType type) : ExpressionNode{ archetype }, type{ type }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
		//setOutput(0, Value{ type, input });
	}

	void drawMiddle() override
	{
		floatFields[0].update();
	}

	void serialize(Serializer& s) override
	{
		ExpressionNode::serialize(s);

		s.serialize("fields", floatFields);
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<ConstantNode>({
			"Constants",
			"const_float",
			"Float",
			{
			},
			{
				{ "", makeValueType<ScalarType>()}
			}
		}, makeValueType<ScalarType>());
	}
};