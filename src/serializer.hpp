#pragma once

#include <string_view>
#include <vector>
#include <set>

#include <nlohmann/json.hpp>

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
		serialize(name).serialize(r, w);
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

void serialize(Serializer& s, float& f)
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

void serialize(Serializer& s, std::uint32_t& f)
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

void serialize(Serializer& s, std::uint64_t& f)
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

void serialize(Serializer& s, std::string& f)
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