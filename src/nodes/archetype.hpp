#pragma once

#include <string>
#include <functional>

#include "graph.hpp"
#include "../value.hpp"

struct NodeArchetype
{
	std::string id;
	std::string title;

	struct Input
	{
		std::string name;
		ValueType type;

		bool optional = false;

		LinkId link{0};
	};

	struct Output
	{
		std::string name;
		ValueType type;
		bool saveToVar = true;
	};

	std::vector<Input> inputs;
	std::vector<Output> outputs;

	std::function<Graph::Node::Ptr()> createNode;

	template<typename T>
	static auto makeArchetype(NodeArchetype archetype)
	{
		auto archetypePtr = std::make_unique<NodeArchetype>(std::move(archetype));
		auto archetypeRawPtr = archetypePtr.get();
		archetypePtr->createNode = [archetypeRawPtr]() { return std::make_unique<T>(archetypeRawPtr); };
		return archetypePtr;
	};
};