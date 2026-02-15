#pragma once

#include "graph.hpp"
#include "nodes/expression.hpp"
#include "value.hpp"

#include <format>
#include <string>
#include <unordered_map>
#include <vector>

struct CodeGenerator
{
    enum class Type
    {
        Vertex,
        Fragment
    };

    Graph& graph;
    Type type;

    static constexpr auto functionPrefix = "mlsf_";

    struct Param
    {
        std::string id;
        ValueType type{Types::scalar};
        bool isOut{};

        bool operator==(const Param&) const = default;
    };

    struct Function
    {
        std::string id;
        ValueType returnType{Types::scalar};
        std::vector<Param> params;
        std::vector<std::string> body;

        bool operator==(const Function&) const = default;

        std::string makeSignature() const
        {
            std::string result;
            const auto returnTypeString = returnType == Types::none ? "void" : returnType.toString();
            result += std::format("{} {}{}(", returnTypeString, functionPrefix, id);

            for (std::size_t x = 0; x < params.size(); x++)
            {
                if (x > 0)
                {
                    result += ", ";
                }

                const auto& param = params[x];

                if (param.isOut)
                {
                    result += "out ";
                }

                result += std::format("{} {}", param.type.toString(), param.id);
            }

            result += ")";

            return result;
        }
    };

    std::unordered_map<std::string, ValueType> shaderInputs;
    std::unordered_map<std::string, Function> functions;
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
                        input.error = std::format("Cannot convert from {} to {}",
                                                  input.value.type.toString(),
                                                  input.type.toString());
                    }

                    input.value = std::move(value);
                }
                else if (input.showField && (input.type.isScalar() || input.type.isVector()))
                {
                    input.field.type = input.type;
                    input.value = input.field.toValue();
                }
            }
        }
        else
        {
            int selectedOverload = -1;
            ValueType genType{};

            for (int overloadIndex{}; overloadIndex < overloads.size(); overloadIndex++)
            {
                const auto& overload = overloads[overloadIndex];

                assert(node.inputs.size() == overload.inputs.size());

                genType = Types::scalar;

                for (uint8_t inputIndex = 0; inputIndex < node.inputs.size(); inputIndex++)
                {
                    const auto& nodeInput = node.inputs[inputIndex];
                    const auto& overloadInput = overload.inputs[inputIndex];
                    if (!overloadInput && genType == Types::scalar && nodeInput.value)
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

            if (selectedOverload < 0)
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

        for (auto& input : node.inputs)
        {
            if (input.requireLink && !input.link)
            {
                input.error = "Pin requires a link";
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

            if (output.value)
            {
                if (output.value.type != Types::texture &&
                    (output.linkCount > 1 || output.value.code.size() >= ExpressionVariableLengthCutoff))
                {
                    setAsVar(node.id.makeOutput(outputIndex), output.value);
                }
                else
                {
                    cachedValues[node.id.makeOutput(outputIndex)] = output.value;
                }
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
                /*
				auto& destNode = graph.getNode<ExpressionNode>(pin.nodeId());
				if (pin.index() < destNode.inputs.size())
				{
					auto& input = destNode.inputs[pin.index()];
					if (input.showField)
					{
						input.field.type = input.type;
						return input.field.toValue();
					}
				}
				*/

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

    void set(PinId pin, Value value)
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
        return {value.type, std::move(varName)};
    }

    Value addEmptyVar(const ValueType& type)
    {
        std::string varName = "var" + std::to_string(nextVar++);
        body.push_back(type.toString() + " " + varName + ";");
        return {type, std::move(varName)};
    }

    void addFunc(const Function& function)
    {
        auto it = functions.find(function.id);
        if (it != functions.end())
        {
            assert(it->second == function);
        }
        else
        {
            functions[function.id] = function;
        }
    }

    Value callFunc(const std::string& id, std::vector<Value> params)
    {
        auto it = functions.find(id);
        assert(it != functions.end());

        const auto& function = it->second;
        assert(params.size() == function.params.size());

        std::string out = functionPrefix + id + "(";
        for (std::size_t x = 0; x < params.size(); x++)
        {
            assert(params[x].type == function.params[x].type);

            if (x > 0)
            {
                out += ", ";
            }

            out += params[x].code;
        }

        out += ")";

        if (function.returnType)
        {
            return {function.returnType, out};
        }
        else
        {
            body.push_back(out + ";");
            return {};
        }
    }

    std::string finalize() const
    {
        std::string code;

        code += "#version 120\n\n";

        for (const auto& [name, type] : shaderInputs)
        {
            code += std::format("uniform {} {};\n", type.toString(), name);
        }

        code += "\n";
        code += "\n";

        for (const auto& [name, func] : functions)
        {
            code += std::format("{}\n{{\n", func.makeSignature());

            for (const auto& line : func.body)
            {
                code += std::format("\t{}\n", line);
            }

            code += "}\n\n";
        }

        code += "\n";

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

void serialize(Serializer& s, CodeGenerator::Param& param)
{
    s.serialize("id", param.id);
    s.serialize("type", param.type);
    s.serialize("isOut", param.isOut);
}

void serialize(Serializer& s, CodeGenerator::Function& function)
{
    s.serialize("id", function.id);
    s.serialize("body", function.body);
    s.serialize("params", function.params);
    s.serialize("returnType", function.returnType);
}