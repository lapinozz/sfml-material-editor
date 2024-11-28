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
		const auto addNode = [&](const auto& cat, const auto& id, const auto& name, const auto overloads, const auto& op)
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
		{/*
			{{Types::scalar, Types::scalar}, {Types::scalar}},
			{{Types::vec2, Types::vec2}, {Types::vec2}},
			{{Types::vec3, Types::vec3}, {Types::vec3}},
			{{Types::vec4, Types::vec4}, {Types::vec4}},
			*/
			{{Types::none, Types::none}, {Types::none}},
		};

		std::vector<NodeArchetype::Overload> overloadsEmpty;

		addNode("Maths", "add", "Add", overloads, Op::Add);
		addNode("Maths", "sub", "Subtract", overloadsEmpty, Op::Sub);
		addNode("Maths", "mul", "Multiply", overloadsEmpty, Op::Mul);
		addNode("Maths", "div", "Divide", overloadsEmpty, Op::Div);

		addNode("Comparison", "less", "<", overloadsEmpty, Op::Less);
		addNode("Comparison", "lessEqual", "<=", overloadsEmpty, Op::LessEqual);
		addNode("Comparison", "greater", ">", overloadsEmpty, Op::Greater);
		addNode("Comparison", "greaterEqual", ">=", overloadsEmpty, Op::GreaterEqual);
		addNode("Comparison", "equal", "==", overloadsEmpty, Op::Equal);
		addNode("Comparison", "logicAnd", "&&", overloadsEmpty, Op::LogicAnd);
		addNode("Comparison", "logicOr", "||", overloadsEmpty, Op::LogicOr);
		addNode("Comparison", "logicExclusiveOr", "^^", overloadsEmpty, Op::LogicExclusiveOr);
	}
};
