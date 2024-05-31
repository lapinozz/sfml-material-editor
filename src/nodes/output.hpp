#pragma once

struct OutputNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	void evaluate(CodeGenerator& generator) override
	{
		//generator.set(id.makeOutput(0), Value{ makeValueType<ScalarType>(), std::to_string(value) }});
	}
};