#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <format>

#include "graph.hpp"
#include "value.hpp"
#include "nodes/expression.hpp"

struct CodeGenerator
{
	enum class Type
	{
		Vertex,
		Fragment
	};

	Graph& graph;
	Type type;

	std::vector<std::string> shaderInputs;
	std::vector<std::string> body;

	std::unordered_map<PinId, Value> cachedValues;

	int nextVar = 0;

	const void evaluate(NodeId nodeId)
	{
		auto& node = graph.findNode<ExpressionNode>(nodeId);
		evaluate(node);
	}

	const void evaluate(ExpressionNode& node)
	{
		for (uint8_t inputIndex = 0; inputIndex < node.inputs.size(); inputIndex++)
		{
			auto& input = node.inputs.at(inputIndex);
			auto value = evaluate(node.id.makeInput(inputIndex));
			if (value && input.type)
			{
				input.value = convert(value, input.type);
				if (!input.value)
				{
					input.error = std::format("Cannot convert from {} to {}", value.type.toString(), input.type.toString());
				}
			}
			else
			{
				input.value = std::move(value);
			}
		}

		node.evaluate(*this);

		for (uint8_t outputIndex = 0; outputIndex < node.outputs.size(); outputIndex++)
		{
			auto& output = node.outputs.at(outputIndex);
			setAsVar(node.id.makeOutput(outputIndex), output.value);
			//cachedValues[node.id.makeOutput(outputIndex)] = output.value;
		}
	}

	const Value& evaluate(PinId pin)
	{
		if (pin.direction() == PinDirection::In)
		{
			auto link = graph.findLink(pin);
			if (link)
			{
				pin = link.from();
			}
			else
			{
				return Values::null;
			}
		}

		{
			const auto it = cachedValues.find(pin);
			if (it != cachedValues.end())
			{
				return it->second;
			}
		}

		const auto nodeId = pin.nodeId();
		evaluate(nodeId);

		{
			const auto it = cachedValues.find(pin);
			if (it != cachedValues.end())
			{
				return it->second;
			}

			return Values::null;
		}
	}

	void set(PinId pin, const Value& value)
	{
		cachedValues[pin] = std::move(value);
	}

	void setAsVar(PinId pin, const Value& value)
	{
		cachedValues[pin] = addVar(value);
	}

	Value addVar(const Value& value)
	{
		std::string varName = "var" + std::to_string(nextVar++);
		body.push_back(value.type.toString() + " " + varName + " = " + value.code + ";");
		return { value.type, std::move(varName) };
	}

	std::string finalize() const
	{
		std::string code;

		code += "#version 120\n";

		for (const auto& str : shaderInputs)
		{
			code += str;
			code += "\n";
		}

		code += "void main()\n{\n";
		for (const auto& str : body)
		{
			code += "\t";
			code += str;
			code += "\n";
		}
		code += "}";

		return code;
	}
};