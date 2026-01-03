#pragma once

#include <string_view>
#include <vector>
#include <array>
#include <set>

#include <nlohmann/json.hpp>

#include "variant-emplace.hpp"

using json = nlohmann::json;

struct Serializer
{
	bool isSaving;
	json& j;

	Serializer() = delete;
	Serializer(bool isSaving, json& j) : isSaving{ isSaving }, j{ j }
	{

	}

	Serializer at(std::string_view name)
	{
		if (isSaving && !j[name].is_object())
		{
			j[name] = json::object();
		}

		return { isSaving, j[name] };
	}

	Serializer at(std::size_t index)
	{
		if (isSaving && !j.is_array())
		{
			j = json::array();
		}

		return { isSaving, j[index] };
	}

	template<typename T>
	void serialize(T& t)
	{
		::serialize(*this, t);
	}

	template<typename T>
	void serialize(std::string_view name, T& t)
	{
		auto ss = at(name);
		::serialize(ss, t);
	}

	template<typename R, typename W>
	void serializeReadWrite(std::string_view name, R&& r, W&& w)
	{
		at(name).serialize(r, w);
	}

	template<typename R, typename W>
	void serializeReadWrite(R&& r, W&& w)
	{
		if (isSaving)
		{
			r(*this);
		}
		else
		{
			w(*this);
		}
	}

	template<typename T, typename R, typename W>
	void serializeValue(std::string_view name, R&& r, W&& w)
	{
		at(name).serializeValue<T>(r, w);
	}

	template<typename T, typename R, typename W>
	void serializeValue(R&& r, W&& w)
	{
		T t;

		if (isSaving)
		{
			t = r();
		}

		serialize(t);

		if (!isSaving)
		{
			w(t);
		}
	}
};

inline void serialize(Serializer& s, json& j)
{
	if (s.isSaving)
	{
		s.j = j;
	}
	else
	{
		j = s.j;
	}
}

inline void serialize(Serializer& s, float& f)
{
	if (s.isSaving)
	{
		s.j = f;
	}
	else
	{
		f = s.j;
	}
}

template <typename T>
requires std::is_arithmetic_v<T>
void serialize(Serializer& s, T& f)
{
	if (s.isSaving)
	{
		s.j = f;
	}
	else
	{
		f = s.j;
	}
}

inline void serialize(Serializer& s, std::string& f)
{
	if (s.isSaving)
	{
		s.j = f;
	}
	else
	{
		f = s.j;
	}
}

template<typename T>
void serialize(Serializer& s, std::vector<T>& v)
{
	if (!s.isSaving)
	{
		v.resize(s.j.size());
	}

	for (std::size_t x = 0; x < v.size(); x++)
	{
		s.at(x).serialize(v[x]);
	}
}

template<typename T, std::size_t S>
void serialize(Serializer& s, std::array<T, S>& v)
{
	for (std::size_t x = 0; x < v.size(); x++)
	{
		s.at(x).serialize(v[x]);
	}
}

template<typename T>
void serialize(Serializer& s, std::set<T>& set)
{
	s.serializeValue<std::vector<T>>
	(
		[&]()
		{
			return std::vector<T>{ set.begin(), set.end() };
		},
		[&](auto& vec)
		{
			set = std::set<T>{ vec.begin(), vec.end() };
		}
	);
}

template<typename Key, typename Value>
void serialize(Serializer& s, std::pair<Key, Value>& pair)
{
	s.at(0).serialize(pair.first);
	s.at(1).serialize(pair.second);
}

template<typename Key, typename Value>
void serialize(Serializer& s, std::unordered_map<Key, Value>& map)
{
	using Pair = std::pair<Key, Value>;

	s.serializeValue<std::vector<Pair>>
	(
		[&]()
		{
			std::vector<Pair> pairs;
			for (const auto& pair : map)
			{
				pairs.push_back(pair);
			}
			return pairs;
		},
		[&](auto& pairs)
		{
			map = {pairs.begin(), pairs.end()};
		}
	);
}

template<typename T>
requires std::is_enum_v<T>
void serialize(Serializer& s, T& e)
{
	serialize(s, reinterpret_cast<std::underlying_type_t<T>&>(e));
}

template<typename...Ts>
void serialize(Serializer& s, std::variant<Ts...>& variant)
{
	if (s.isSaving)
	{
		std::size_t variantIndex = variant.index();
		s.serialize("index", variantIndex);
	}
	else
	{
		std::size_t variantIndex;
		s.serialize("index", variantIndex);
		variantEmplace(variant, variantIndex);
	}

	std::visit([&](auto& value)
	{
		s.serialize("value", value);
	}, variant);
}