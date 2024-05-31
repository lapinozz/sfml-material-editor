#pragma once

struct AddNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	FloatField fieldA;
	FloatField fieldB;

	void evaluate(CodeGenerator& generator) override
	{
		const auto& a = generator.evaluate(id.makeInput(0));
		const auto& b = generator.evaluate(id.makeInput(1));


		const auto& aVal = a ? a : fieldA.toValue();
		const auto& bVal = b ? b : fieldB.toValue();

		assert(aVal.type == bVal.type);

		generator.setAsVar(id.makeOutput(0), Value{ aVal.type, aVal.code + " + " + bVal.code });
	}

	void drawInputPin(const NodeArchetype::Input& input, PinId id) override
	{
		ExpressionNode::drawInputPin(input, id);

		if (!input.link)
		{
			ImGui::SameLine();

			id.index() == 0 ? fieldA.update() : fieldB.update();
		}
	};

	void serialize(Serializer& s) override
	{
		ExpressionNode::serialize(s);

		s.serialize("fieldA", fieldA);
		s.serialize("fieldB", fieldB);
	}
};
