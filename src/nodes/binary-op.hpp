#pragma once

#include <array>

struct BinaryOpNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	enum class Op
	{
		Add,
		Sub,
		Mul,
		Div,
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Equal,
		Different,
		LogicAnd,
		LogicOr,
		LogicExclusiveOr,
	};

	static inline std::array operatorStrings = {
		"+",
		"-",
		"*",
		"/",
		"<",
		"<=",
		">",
		">=",
		"==",
		"!=",
		"&&",
		"!!",
		"^^",
	};

	Op op;

	FloatField fieldA;
	FloatField fieldB;

	BinaryOpNode(NodeArchetype* archetype, Op op) : ExpressionNode{ archetype }, op{ op }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
		const auto& a = getInput(0);
		const auto& b = getInput(1);


		const auto& aVal = a ? a : fieldA.toValue();
		const auto& bVal = b ? b : fieldB.toValue();

		assert(aVal.type == bVal.type);

		setOutput(0, Value{ aVal.type, aVal.code + " " + operatorStrings[std::to_underlying(op)] + " " + bVal.code });
	}

	void drawInputPin(const Input& input, PinId id) override
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

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		const auto addNode = [&](const auto& cat, const auto& id, const auto& name, const auto& op)
		{
			repo.add<BinaryOpNode>({
				cat,
				id,
				name,
				{
					{ "A", makeValueType<ScalarType>()},
					{ "B", makeValueType<ScalarType>()}
				},
				{
					{ "", makeValueType<ScalarType>()},
				}
			}, op);
		};

		addNode("Maths", "add", "Add", Op::Add);
		addNode("Maths", "sub", "Subtract", Op::Sub);
		addNode("Maths", "mul", "Multiply", Op::Mul);
		addNode("Maths", "div", "Divide", Op::Div);

		addNode("Comparison", "less", "<", Op::Less);
		addNode("Comparison", "lessEqual", "<=", Op::LessEqual);
		addNode("Comparison", "greater", ">", Op::Greater);
		addNode("Comparison", "greaterEqual", ">=", Op::GreaterEqual);
		addNode("Comparison", "equal", "==", Op::Equal);
		addNode("Comparison", "logicAnd", "&&", Op::LogicAnd);
		addNode("Comparison", "logicOr", "||", Op::LogicOr);
		addNode("Comparison", "logicExclusiveOr", "^^", Op::LogicExclusiveOr);
	}
};
