#pragma once

#include <variant>
#include <string>

struct NoneType
{
	bool operator==(const NoneType&) const = default;
};

struct ScalarType
{
	bool operator==(const ScalarType&) const = default;
};

struct VectorType
{
	uint8_t arrity;
	bool operator==(const VectorType&) const = default;
};

struct MatrixType
{
	uint8_t x;
	uint8_t y;

	bool operator==(const MatrixType&) const = default;
};

struct SamplerType
{
	bool operator==(const SamplerType&) const = default;
};

struct ValueType : std::variant<NoneType, ScalarType, VectorType, MatrixType, SamplerType>
{
	std::uint16_t arrity = 0;

	constexpr ValueType() : variant{ NoneType{} }
	{

	}

	template<typename...Args>
	constexpr ValueType(std::uint16_t arrity, Args...args) : arrity{ arrity }, variant{ args... }
	{

	}

	bool operator==(const ValueType&) const = default;

	operator bool() const
	{
		return index() != 0;
	}

	std::string toString() const
	{
		assert(index() > 0);
		if (const auto* t = std::get_if<ScalarType>(this))
		{
			return "float";
		}

		assert(false);
	}
};

template<typename T, typename...Args>
constexpr ValueType makeValueType(Args...args)
{
	return { 0, T{args...} };
}

template<typename T, typename...Args>
constexpr ValueType makeArrayValueType(std::uint16_t arrity, Args...args)
{
	return { arrity, T{args...} };
}

struct Value
{
	ValueType type;
	std::string code;

	operator bool() const
	{
		return type;
	}

	Value value_or(const auto& other) const
	{
		return *this ? *this : other;
	}
};

namespace Values
{
	static inline Value null{};
	static inline Value zero{makeValueType<ScalarType>(), "0.0f"};
}

Value convert(const Value& value, const ValueType& type)
{
	if (value.type == type)
	{
		return value;
	}

	if (auto* t1 = std::get_if<ScalarType>(&value.type))
	{
		if (auto* t2 = std::get_if<VectorType>(&type))
		{
			std::string code;
			code += "vec";
			code += std::to_string(t2->arrity);
			code += "(";
			for (std::size_t x = 0; x < t2->arrity - 1; x++)
			{
				code += value.code;
				code += ",";
			}
			code += value.code;
			code += ")";
			return { type, code };
		}
	}

	return Values::null;
}