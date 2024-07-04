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

		bool optional = false;

		LinkId link{0};
	};

	struct Output
	{
		std::string name;
		ValueType type;
		bool saveToVar = true;

		LinkId link{ 0 };
	};

	std::vector<Input> inputs;
	std::vector<Output> outputs;

	std::function<Graph::Node::Ptr()> createNode;
};