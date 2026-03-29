#include "graph.hpp"
#include "nodes/expression.hpp"
#include "value.hpp"
#include "string-utils.hpp"

#include <format>
#include <string>
#include <unordered_map>
#include <vector>

namespace CodeGen
{

constexpr auto functionPrefix = "mlsf_";

struct Param
{
    std::string id;
    ValueType type{Types::scalar};
    bool isOut{};

    bool operator==(const Param&) const = default;
};

struct Function
{
    std::string id;
    ValueType returnType{Types::scalar};
    std::vector<Param> params;
    std::vector<std::string> body;
    std::vector<std::string> dependencies;

    bool operator==(const Function&) const = default;

    std::string makeSignature() const
    {
        std::string result;
        const auto returnTypeString = returnType == Types::none ? "void" : returnType.toString();
        result += std::format("{} {}{}(", returnTypeString, functionPrefix, id);

        for (std::size_t x = 0; x < params.size(); x++)
        {
            if (x > 0)
            {
                result += ", ";
            }

            const auto& param = params[x];

            if (param.isOut)
            {
                result += "out ";
            }

            result += std::format("{} {}", param.type.toString(), param.id);
        }

        result += ")";

        return result;
    }
};

constexpr std::array<Function, 4> makeGeneric(const Function base)
{
    std::array<ValueType, 4> types{Types::scalar, Types::vec2, Types::vec3, Types::vec4};
    std::array<std::string, 4> swizzles{"x", "xy", "xyz", "xyzw"};
    std::array<std::string, 4> postfix{"_f", "_vec2", "_vec3", "_vec4"};

    const auto replaceStrings = [&](std::string str, int x) {
        str = StringUtils::replaceAll(str, "$TYPE$", types[x].toString());
        str = StringUtils::replaceAll(str, "$SWIZZLE$", swizzles[x]);
        str = StringUtils::replaceAll(str, "$PRE$", functionPrefix);
        str = StringUtils::replaceAll(str, "$POST$", postfix[x]);
        return str;
    };

    std::array<Function, 4> result;
    for (int x = 0 ; x < 4; x++)
    {
        auto& func = result[x];
        func = base;

        func.id += postfix[x];

        if (func.returnType == Types::none)
        {
            func.returnType = types[x];
        }

        for (auto& param : func.params)
        {
            if (param.type == Types::none)
            {
                param.type = types[x];
            }
        }
        
        for (auto& line : func.body)
        {
            line = replaceStrings(line, x);
        }

        for (auto& dep : func.dependencies)
        {
            dep = replaceStrings(dep, x);
        }
    }

    return result;
}

} // namespace CodeGen