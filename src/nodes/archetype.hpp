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
};