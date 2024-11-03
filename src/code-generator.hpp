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
		auto& node = graph.getNode<ExpressionNode>(nodeId);
		evaluate(node);
	}

	const void evaluate(ExpressionNode& node)
	{
		for (uint8_t inputIndex = 0; inputIndex < node.inputs.size(); inputIndex++)
		{
			auto& input = node.inputs.at(inputIndex);
			input.value = evaluate(node.id.makeInput(inputIndex));
		}

		const auto& archetype = *node.archetype;
		const auto& overloads = archetype.overloads;
		if (overloads.empty())
		{
			for (uint8_t inputIndex = 0; inputIndex < node.inputs.size(); inputIndex++)
			{
				auto& input = node.inputs.at(inputIndex);
				if (input.value && input.type)
				{
					auto value = convert(input.value, input.type);
					if (!value)
					{
						input.error = std::format("Cannot convert from {} to {}", input.value.type.toString(), input.type.toString());
					}

					input.value = std::move(value);
				}
			}
		}
		else
		{
			int selectedOverload = -1;
			ValueType genType{};

			for (size_t overloadIndex{}; overloadIndex < overloads.size(); overloadIndex++)
			{
				const auto& overload = overloads[overloadIndex];

				assert(node.inputs.size() == overload.inputs.size());

				genType = Types::scalar;

				for (uint8_t inputIndex = 0; inputIndex < node.inputs.size(); inputIndex++)
				{
					const auto& nodeInput = node.inputs[inputIndex];
					const auto& overloadInput = overload.inputs[inputIndex];
					if(!overloadInput && genType == Types::scalar && nodeInput.value)
					{
						genType = nodeInput.value.type;
					}
				}

				bool matches = true;
				for (uint8_t inputIndex = 0; inputIndex < node.inputs.size(); inputIndex++)
				{
					const auto& nodeInput = node.inputs[inputIndex];
					if (!nodeInput.value)
					{
						continue;
					}

					const auto& overloadInput = overload.inputs[inputIndex];
					const auto targetType = overloadInput ? overloadInput : genType;

					if (!canConvert(nodeInput.value.type, targetType))
					{
						matches = false;
						break;
					}
				}

				if (matches)
				{
					selectedOverload = overloadIndex;
					break;
				}
			}

			if(selectedOverload < 0)
			{
				node.inputs[0].error = "No compatible overload found";
			}
			else
			{
				const auto& overload = overloads[selectedOverload];
				for (uint8_t inputIndex = 0; inputIndex < node.inputs.size(); inputIndex++)
				{
					auto& nodeInput = node.inputs[inputIndex];

					const auto& overloadInput = overload.inputs[inputIndex];
					const auto targetType = overloadInput ? overloadInput : genType;
					nodeInput.type = targetType;

					if (nodeInput.value)
					{
						nodeInput.value = convert(nodeInput.value, nodeInput.type);
					}
					else if (std::holds_alternative<GenType>(nodeInput.type))
					{
						nodeInput.field.type = nodeInput.type;
						nodeInput.value = nodeInput.field.toValue();
					}
				}

				for (uint8_t outputIndex = 0; outputIndex < node.outputs.size(); outputIndex++)
				{
					auto& nodeOutput = node.outputs[outputIndex];

					const auto& overloadOutput = overload.outputs[outputIndex];
					const auto targetType = overloadOutput ? overloadOutput : genType;
					nodeOutput.type = targetType;
				}
			}
		}

		node.evaluate(*this);

		constexpr size_t ExpressionVariableLengthCutoff = 15;

		for (uint8_t outputIndex = 0; outputIndex < node.outputs.size(); outputIndex++)
		{
			const auto& output = node.outputs.at(outputIndex);

			if (output.linkCount <= 0)
			{
				continue;
			}

			if (output.linkCount > 1 || output.value.code.size() >= ExpressionVariableLengthCutoff)
			{
				setAsVar(node.id.makeOutput(outputIndex), output.value);
			}
			else
			{
				cachedValues[node.id.makeOutput(outputIndex)] = output.value;
			}
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
		}

		return Values::null;
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