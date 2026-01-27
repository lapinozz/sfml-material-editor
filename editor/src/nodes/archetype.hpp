#pragma once

#include <string>
#include <functional>

#include "graph.hpp"
#include "../value.hpp"

struct NodeArchetype
{
	std::string category;
	std::string id;
	std::string title;

	struct Input
	{
		std::string name;
		ValueType type;

		bool requireLink = false;
		bool showField = true;
	};

	struct Output
	{
		std::string name;
		ValueType type;
	};

	std::vector<Input> inputs;
	std::vector<Output> outputs;

	struct Overload
	{
		std::vector<ValueType> inputs;
		std::vector<ValueType> outputs;
	};

	std::vector<Overload> overloads;

	std::function<Graph::Node::Ptr()> createNode;

	/*
	static std::optional<std::pair<PinId, PinId>> findConnectTarget(const NodeArchetype& out, const NodeArchetype& in)
	{
		const auto& canTarget = [&](ValueType outType, ValueType inType)
		{
			if (canConvert(outType, inType))
			{
				return true;
			}

			if (!outType && inType.isGenType())
			{
				return true;
			}

			if (outType.isGenType() && !inType)
			{
				return true;
			}

			return false;
		};

		for (int outIndex{}; outIndex < out.outputs.size(); outIndex++)
		{
			for (int inputIndex{}; inputIndex < in.inputs.size(); inputIndex++)
			{
				if (canTarget(out.outputs[outIndex].type, in.inputs[inputIndex].type))
				{
					return std::pair<PinId, PinId>{outIndex, inputIndex};
				}
			}

			for (const auto& inOverload : in.overloads)
			{
				for (int inputIndex{}; inputIndex < inOverload.inputs.size(); inputIndex++)
				{
					if (canTarget(out.outputs[outIndex].type, inOverload.inputs[inputIndex]))
					{
						return std::pair<PinId, PinId>{outIndex, inputIndex};
					}
				}
			}
		}

		for (const auto& outOverload : out.overloads)
		{
			for (int outIndex{}; outIndex < outOverload.outputs.size(); outIndex++)
			{
				for (int inputIndex{}; inputIndex < in.inputs.size(); inputIndex++)
				{
					if (canTarget(outOverload.outputs[outIndex], in.inputs[inputIndex].type))
					{
						return std::pair<PinId, PinId>{outIndex, inputIndex};
					}
				}

				for (const auto& inOverload : in.overloads)
				{
					for (int inputIndex{}; inputIndex < inOverload.inputs.size(); inputIndex++)
					{
						if (canTarget(outOverload.outputs[outIndex], inOverload.inputs[inputIndex]))
						{
							return std::pair<PinId, PinId>{outIndex, inputIndex};
						}
					}
				}
			}
		}

		return std::nullopt;
	}

	static PinId findConnectTarget(const NodeArchetype& out, PinId In)
	{
		const auto& canTarget = [&](ValueType outType, ValueType inType)
		{
			if (canConvert(outType, inType))
			{
				return true;
			}

			if (!outType && inType.isGenType())
			{
				return true;
			}

			if (outType.isGenType() && !inType)
			{
				return true;
			}

			return false;
		};

		for (int outIndex{}; outIndex < out.outputs.size(); outIndex++)
		{
			for (int inputIndex{}; inputIndex < in.inputs.size(); inputIndex++)
			{
				if (canTarget(out.outputs[outIndex].type, in.inputs[inputIndex].type))
				{
					return std::pair<PinId, PinId>{outIndex, inputIndex};
				}
			}

			for (const auto& inOverload : in.overloads)
			{
				for (int inputIndex{}; inputIndex < inOverload.inputs.size(); inputIndex++)
				{
					if (canTarget(out.outputs[outIndex].type, inOverload.inputs[inputIndex]))
					{
						return std::pair<PinId, PinId>{outIndex, inputIndex};
					}
				}
			}
		}

		for (const auto& outOverload : out.overloads)
		{
			for (int outIndex{}; outIndex < outOverload.outputs.size(); outIndex++)
			{
				for (int inputIndex{}; inputIndex < in.inputs.size(); inputIndex++)
				{
					if (canTarget(outOverload.outputs[outIndex], in.inputs[inputIndex].type))
					{
						return std::pair<PinId, PinId>{outIndex, inputIndex};
					}
				}

				for (const auto& inOverload : in.overloads)
				{
					for (int inputIndex{}; inputIndex < inOverload.inputs.size(); inputIndex++)
					{
						if (canTarget(outOverload.outputs[outIndex], inOverload.inputs[inputIndex]))
						{
							return std::pair<PinId, PinId>{outIndex, inputIndex};
						}
					}
				}
			}
		}

		return std::nullopt;
	}
	*/
};