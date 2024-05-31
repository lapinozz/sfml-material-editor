#pragma once

struct CodeGenerator
{
	Graph& graph;

	std::vector<std::string> shaderInputs;
	std::vector<std::string> body;

	Value nullVall;
	std::unordered_map<PinId, Value> cachedValues;

	int nextVar = 0;

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
				return nullVall;
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
		auto& node = graph.findNode<ExpressionNode>(nodeId);

		node.evaluate(*this);

		{
			const auto it = cachedValues.find(pin);
			if (it != cachedValues.end())
			{
				return it->second;
			}

			assert(false);
		}
	}

	void set(PinId pin, Value value)
	{
		cachedValues[pin] = std::move(value);
	}

	void setAsVar(PinId pin, Value value)
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
		//code += shaderInputs;
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