#pragma once


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

struct ValueType
{
	std::uint16_t arrity;
	std::variant<NoneType, ScalarType, VectorType, MatrixType, SamplerType> type;

	bool operator==(const ValueType&) const = default;

	operator bool() const
	{
		return type.index() != 0;
	}

	std::string toString() const
	{
		assert(type.index() > 0);
		if (auto* t = std::get_if<ScalarType>(&type))
		{
			return "float";
		}
	}
};

template<typename T, typename...Args>
constexpr ValueType makeValueType(std::uint16_t arrity = 1, Args...args)
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
};