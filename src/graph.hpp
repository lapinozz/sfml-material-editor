#pragma once

#include <set>

#include <imgui_node_editor.h>
#include <imgui_node_editor_internal.h>

#include "id-pool.hpp"

namespace ed = ax::NodeEditor;

template <typename T, typename Tag>
struct SafeId
{
	SafeId(T t) : value{ t }
	{
	}

	SafeId(const SafeId&) = default;

	template <typename T2, typename Tag2>
	SafeId(
		const SafeId
		<
		typename std::enable_if<!std::is_same<T, T2>::value, T2>::type,
		typename std::enable_if<!std::is_same<Tag, Tag2>::value, Tag2>::type
		>&) = delete;

	SafeId& operator=(const SafeId&) = default;

	bool operator==(const SafeId&) const = default;
	auto operator<=>(const SafeId&) const = default;

	explicit operator T() const { return Get(); }

	T Get() const { return value; }

	void serialize(Serializer& s)
	{
		s.serialize(value);
	};

private:
	T value;
};

namespace std
{
	template <typename T, typename Tag>
	struct hash<SafeId<T, Tag>>
	{
		size_t operator()(const SafeId<T, Tag>& id) const noexcept
		{
			return std::hash(id.Get());
		}
	};
}

using ShortId = std::uint32_t;
using LongId = std::uint64_t;

struct NodeId;

enum class PinDirection
{
	In,
	Out
};

struct PinId : SafeId<ShortId, PinId>
{
	using SafeId::SafeId;

	PinId(ed::PinId id) : PinId(id.Get())
	{
	}

	operator ed::PinId() const
	{
		return Get();
	}

	using PinIndex = std::uint8_t;

	static constexpr ShortId pinIndexOffset = (sizeof(ShortId) - 1) * 8;
	static constexpr ShortId pinDirOffset = pinIndexOffset + 7;
	static constexpr ShortId pinIndexMask = 0x7F << pinIndexOffset;
	static constexpr ShortId pinDirMask = 0x80 << pinIndexOffset;
	static constexpr ShortId pinNodeMask = static_cast<ShortId>(-1) & ~pinIndexMask & ~pinDirMask;

	static PinId makeInput(NodeId nodeId, PinIndex index);
	static PinId makeOutput(NodeId nodeId, PinIndex index);

	PinDirection direction() const;
	PinIndex index() const;
	NodeId nodeId() const;
};

struct NodeId : SafeId<ShortId, NodeId>
{
	using SafeId::SafeId;

	NodeId(ed::NodeId id) : SafeId(id.Get())
	{
	}

	operator ed::NodeId() const
	{
		return Get();
	}

	operator bool() const
	{
		return Get();
	}

	PinId makeInput(PinId::PinIndex index)
	{
		return PinId::makeInput(*this, index);
	}

	PinId makeOutput(PinId::PinIndex index)
	{
		return PinId::makeOutput(*this, index);
	}
};

PinId PinId::makeInput(NodeId nodeId, PinIndex index)
{
	return nodeId.Get() | ((index + 1) << pinIndexOffset);
}

PinId PinId::makeOutput(NodeId nodeId, PinIndex index)
{
	return nodeId.Get() | ((index + 1) << pinIndexOffset) | pinDirMask;
}

PinDirection PinId::direction() const
{
	return (Get() & pinDirMask) ? PinDirection::Out : PinDirection::In;
}

PinId::PinIndex PinId::index() const
{
	return ((Get() & pinIndexMask) >> pinIndexOffset) - 1;
}

NodeId PinId::nodeId() const
{
	return Get() & pinNodeMask;
}

struct LinkId : SafeId<LongId, LinkId>
{
	using SafeId::SafeId;

	LinkId(ed::LinkId id) : LinkId(id.Get())
	{
	}

	operator ed::LinkId() const
	{
		return Get();
	}

	operator bool() const
	{
		return Get();
	}

	static LongId make(PinId from, PinId to)
	{
		assert(from.direction() != to.direction());
		if (to.direction() == PinDirection::Out)
		{
			std::swap(from, to);
		}

		return from.Get() | (static_cast<LongId>(to.Get()) << 32);
	}

	LinkId() : SafeId{ 0 }
	{

	}

	LinkId(PinId from, PinId to) : SafeId{ make(from, to) }
	{
	}

	PinId from() const
	{
		return Get() & 0xFFFFFFFF;
	}

	PinId to() const
	{
		return Get() >> 32;
	}
};

namespace std
{
	template <>
	struct hash<PinId>
	{
		size_t operator()(const PinId& id) const noexcept
		{
			return std::hash<ShortId>{}(id.Get());
		}
	};

	template <>
	struct hash<NodeId>
	{
		size_t operator()(const PinId& id) const noexcept
		{
			return std::hash<ShortId>{}(id.Get());
		}
	};

	template <>
	struct hash<LinkId>
	{
		size_t operator()(const PinId& id) const noexcept
		{
			return std::hash<LongId>{}(id.Get());
		}
	};
}

std::ostream& operator<<(std::ostream& stream, PinId id)
{
	return stream << "(" << id.nodeId().Get() << "[" << +id.index() << "]" << (id.direction() == PinDirection::In ? '<' : '>') << ')';
}

struct Graph
{
	struct Node
	{
		NodeId id{ 0 };

		using Ptr = std::unique_ptr<Node>;

		virtual ~Node() = default;
	};

	IdPool<ShortId> idPool;

	std::vector<Node::Ptr> nodes;
	std::set<LinkId> links;

	Node& addNode(Node::Ptr&& n)
	{
		if (!n->id)
		{
			n->id = idPool.take();
		}
		return *nodes.emplace_back(std::move(n));
	}

	template<typename T>
	T& findNode(NodeId id)
	{
		auto it = std::ranges::find_if(nodes, [&](auto& node) {return id == node->id; });
		assert(it != nodes.end());

		auto* ptr = dynamic_cast<T*>(it->get());
		assert(ptr);

		return *ptr;
	}

	void addLink(PinId in, PinId out)
	{
		links.emplace(LinkId{ out, in });
	}

	void removeLink(LinkId id)
	{
		auto it = std::find(links.begin(), links.end(), id);
		if (it == links.end())
		{
			return;
		}

		it = links.erase(it);
	}

	void removeLinks(NodeId id)
	{
		auto it = links.begin();
		while (true)
		{
			it = std::find_if(it, links.end(), [&](const auto& link) {return id == link.from().nodeId() || id == link.to().nodeId(); });
			if (it == links.end())
			{
				break;
			}

			it = links.erase(it);
		}
	}

	LinkId findLink(PinId id) const
	{
		if (id.direction() == PinDirection::In)
		{
			auto it = links.lower_bound(LinkId{ NodeId{0}.makeOutput(0), id });
			if (it != links.end() && it->to() == id)
			{
				return *it;
			}
		}
		else
		{
			auto it = std::ranges::find_if(links, [&](auto& link) {return link.from() == id; });
			if (it != links.end())
			{
				return *it;
			}
		}

		return LinkId{0};
	}

	void removeNode(NodeId id)
	{
		auto it = std::ranges::find_if(nodes, [&](auto& node) {return id == node->id; });
		if (it == nodes.end())
		{
			return;
		}

		auto& node = **it;
		
		removeLinks(id);

		nodes.erase(it);
	}

	bool canAddLink(PinId in, PinId out)
	{
		if (in == out)
		{
			return false;
		}

		if (in.direction() == out.direction())
		{
			return false;
		}

		if (in.nodeId() == out.nodeId())
		{
			return false;
		}

		if (std::ranges::find(links, LinkId{ out, in }) != links.end())
		{
			return false;
		}

		return true;
	}
};

template<typename T, typename Tag>
void serialize(Serializer& s, SafeId<T, Tag>& i)
{
	i.serialize(s);
};