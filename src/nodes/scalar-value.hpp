#pragma once

#include <array>

#include "archetypes.hpp"
#include "expression.hpp"

struct ScalarValueNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	FloatField floatField;

	void evaluate(CodeGenerator& generator) override
	{
		setOutput(0, floatField.toValue());
	}

	void drawInputPins() override
	{
		floatField.update();
	}

	void serialize(Serializer& s) override
	{
		ExpressionNode::serialize(s);

		s.serialize("field", floatField);
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<ScalarValueNode>({
			"Constants",
			"scalar",
			"Scalar",
			{},
			{
				{ "", makeValueType<ScalarType>()}
			}
		});
	}
};