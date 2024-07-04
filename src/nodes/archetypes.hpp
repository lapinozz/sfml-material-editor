#pragma once

#include <unordered_map>

#include "serializer.hpp"
#include "archetype.hpp"
#include "expression.hpp"

struct ArchetypeRepo
{
	ArchetypeRepo() = default;
	ArchetypeRepo(ArchetypeRepo&&) = default;
	ArchetypeRepo(const ArchetypeRepo&) = delete;

	std::unordered_map<std::string, NodeArchetype> archetypes;

	const NodeArchetype& get(const std::string& id)
	{
		return archetypes[id];
	}

	template<typename T, typename...Args>
	const NodeArchetype& add(NodeArchetype archetype, Args...args)
	{
		auto& arch = archetypes.emplace(archetype.id, std::move(archetype)).first->second;
		auto archetypeRawPtr = &arch;
		arch.createNode = [=]() { return std::make_unique<T>(archetypeRawPtr, args...); };

		return arch;
	}
};

struct NodeSerializer
{
	static inline ArchetypeRepo* repo{};
	static void serialize(Serializer& s, Graph::Node::Ptr& n)
	{
		assert(repo);

		if (s.isSaving)
		{
			auto& node = static_cast<ExpressionNode&>(*n);
			s.serialize("type_id", node.archetype->id);
			node.serialize(s);
		}
		else
		{
			std::string typeId;
			s.serialize("type_id", typeId);

			n = repo->get(typeId).createNode();

			auto& node = static_cast<ExpressionNode&>(*n);
			node.serialize(s);
		}
	}
};