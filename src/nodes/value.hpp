#pragma once

#include "expression.hpp"

struct ValueNode : ExpressionNode
{
	FloatField field;

	using ExpressionNode::ExpressionNode;

	void evaluate(CodeGenerator& generator) override
	{
		generator.set(id.makeOutput(0), field.toValue());
	}

	void drawMiddle() override
	{
		field.update();
	}

	void serialize(Serializer& s) override
	{
		ExpressionNode::serialize(s);

		s.serialize("field", field);
	}
};