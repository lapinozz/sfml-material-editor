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

	BinaryOpNode(NodeArchetype* archetype, Op op) : ExpressionNode{ archetype }, op{ op }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
		auto& ia = inputs.at(0);
		auto& ib = inputs.at(1);

		const auto& a = getInput(0);
		const auto& b = getInput(1);

		setOutput(0, Value{ outputs.at(0).type, a.code + " " + operatorStrings[std::to_underlying(op)] + " " + b.code});
	}

	void serialize(Serializer& s) override
	{
		ExpressionNode::serialize(s);
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		const auto addNode = [&](const auto& cat, const auto& id, const auto& name, const auto& overloads, const auto& op)
		{
			repo.add<BinaryOpNode>({
				cat,
				id,
				name,
				{
					{ "A", makeValueType<NoneType>()},
					{ "B", makeValueType<NoneType>()}
				},
				{
					{ "", makeValueType<NoneType>()},
				},
				overloads
			}, op);
		};

		std::vector<NodeArchetype::Overload> overloads
		{
			{{Types::none, Types::scalar}, {Types::none}},
			{{Types::none, Types::none}, {Types::none}},
		};

		std::vector<NodeArchetype::Overload> overloadsEmpty;

		addNode("Maths", "add", "Add", overloads, Op::Add);
		addNode("Maths", "sub", "Subtract", overloads, Op::Sub);
		addNode("Maths", "mul", "Multiply", overloads, Op::Mul);
		addNode("Maths", "div", "Divide", overloads, Op::Div);

		std::vector<NodeArchetype::Overload> overloadsComp
		{
			{{Types::scalar, Types::scalar}, {Types::scalar}},
		};

		addNode("Comparison", "less", "<", overloadsComp, Op::Less);
		addNode("Comparison", "lessEqual", "<=", overloadsComp, Op::LessEqual);
		addNode("Comparison", "greater", ">", overloadsComp, Op::Greater);
		addNode("Comparison", "greaterEqual", ">=", overloadsComp, Op::GreaterEqual);

		addNode("Comparison", "equal", "==", overloadsComp, Op::Equal);
		addNode("Comparison", "logicAnd", "&&", overloadsComp, Op::LogicAnd);
		addNode("Comparison", "logicOr", "||", overloadsComp, Op::LogicOr);
		addNode("Comparison", "logicExclusiveOr", "^^", overloadsComp, Op::LogicExclusiveOr);
	}
};
