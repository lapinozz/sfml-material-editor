#pragma once

#include <variant>
#include <string>
#include <format>

struct NoneType
{
	bool operator==(const NoneType&) const = default;
};

struct GenType
{
	uint8_t arrity{};
	bool operator==(const GenType&) const = default;
};

struct MatrixType
{
	uint8_t x{};
	uint8_t y{};

	bool operator==(const MatrixType&) const = default;
};

struct SamplerType
{
	bool operator==(const SamplerType&) const = default;
};

struct ArrayType
{
	//std::unique_ptr<struct ValueType> innerType;
	bool operator==(const ArrayType&) const = default;
};

struct ValueType : std::variant<NoneType, GenType, MatrixType, SamplerType, ArrayType>
{
	constexpr ValueType() : variant{ NoneType{} }
	{

	}

	template<typename...Args>
	constexpr ValueType(Args...args) : variant{ args... }
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
		if (const auto* t = std::get_if<GenType>(this))
		{
			if (t->arrity == 1)
			{
				return "float";
			}
			else
			{
				return std::format("vec{}", t->arrity);
			}
		}

		assert(false);
	}

	ImColor toColor() const
	{
		if (const auto* t = std::get_if<NoneType>(this))
		{
			return {0.66f, 0.66f, 0.66f, 1.f};
		}
		else if (const auto* t = std::get_if<GenType>(this))
		{
			if (t->arrity == 1)
			{
				return { 45, 75, 196, 255 };
			}
			else
			{
				return { 57, 206, 112, 255 };
			}
		}

		assert(false);
	}

	bool isScalar() const
	{
		if (const auto* t = std::get_if<GenType>(this))
		{
			return t->arrity == 1;
		}

		return false;
	}

	bool isVector() const
	{
		if (const auto* t = std::get_if<GenType>(this))
		{
			return t->arrity > 1;
		}

		return false;
	}
};

template<typename T, typename...Args>
constexpr ValueType makeValueType(Args...args)
{
	return T{args...};
}

template<typename T, typename...Args>
constexpr ValueType makeArrayValueType(std::uint16_t arrity, Args...args)
{
	return { ArrayType{std::make_unique<ValueType>(T{args...})}};
}

namespace Types
{
	static inline ValueType none{ makeValueType<NoneType>() };
	static inline ValueType scalar{ makeValueType<GenType>(uint8_t{1}) };
	static inline ValueType vec2{ makeValueType<GenType>(uint8_t{2}) };
	static inline ValueType vec3{ makeValueType<GenType>(uint8_t{3}) };
	static inline ValueType vec4{ makeValueType<GenType>(uint8_t{4}) };

	ValueType makeVec(uint8_t arrity)
	{
		return makeValueType<GenType>(arrity);
	};
}

bool canConvert(ValueType from, ValueType to)
{
	if (from == to)
	{
		return true;
	}

	if (from == Types::scalar)
	{
		if (to != Types::scalar && std::holds_alternative<GenType>(to))
		{
			return true;
		}
	}

	return false;
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
	static inline Value zero{Types::scalar, "0.0f"};
}

Value convert(const Value& value, const ValueType& type)
{
	if (value.type == type)
	{
		return value;
	}

	if (value.type == Types::scalar)
	{
		if (auto* t2 = std::get_if<GenType>(&type))
		{
			std::string code;
			code += "vec";
			code += std::to_string(t2->arrity);
			code += "(";
			for (std::size_t x = 0; x < t2->arrity - 1; x++)
			{
				//code += value.code;
				//code += ",";
			}
			code += value.code;
			code += ")";
			return { type, code };
		}
	}

	return Values::null;
}