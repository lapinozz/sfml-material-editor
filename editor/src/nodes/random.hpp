#pragma once

#include "ViewportScopeGuard.hpp"
#include "archetypes.hpp"
#include "code-generator.hpp"
#include "expression.hpp"
#include "mls/material.hpp"

#include <array>
#include <format>

struct RandomNode : ExpressionNode
{
    using ExpressionNode::ExpressionNode;

    uint8_t arrity;

    RandomNode(NodeArchetype* archetype, uint8_t arrity) : ExpressionNode{archetype}, arrity{arrity}
    {
    }

    void update(GraphContext* inGraphContext) override
    {
        ExpressionNode::update(inGraphContext);
    }

    static inline const std::array<CodeGen::Function, 4> randFuncs = CodeGen::makeGeneric({
        .id = "rand",
        .returnType = Types::scalar,
        .params = {{"v", Types::none}},
        .body = {"return fract(sin(dot(v + 23.208, vec4(12.9898, 78.233, 45.164, 94.673).$SWIZZLE$)) * 43758.5453);"},
    });

    static inline const std::array<CodeGen::Function, 4> rand2Funcs = CodeGen::makeGeneric({
        .id = "rand2",
        .returnType = Types::vec2,
        .params = {{"v", Types::none}},
        .body =
            {
                "float a = fract(sin(dot(v + 97.541, vec4(34.367, 18.558, 94.370, 44.459).$SWIZZLE$)) * 25961.34723);",
                "float b = fract(sin(dot(v + 72.274, vec4(70.497, 58.662, 72.637, 17.475).$SWIZZLE$)) * 54343.90005);",
                "return vec2(a, b);",
            },
    });

    static inline const std::array<CodeGen::Function, 4> rand3Funcs = CodeGen::makeGeneric({
        .id = "rand3",
        .returnType = Types::vec3,
        .params = {{"v", Types::none}},
        .body =
            {
                "float a = fract(sin(dot(v + 70.497, vec4(84.728, 85.040, 44.166, 51.316).$SWIZZLE$)) * 26283.45416);",
                "float b = fract(sin(dot(v + 28.852, vec4(58.910, 34.142, 26.222, 16.950).$SWIZZLE$)) * 29288.86063);",
                "float c = fract(sin(dot(v + 26.805, vec4(99.266, 84.077, 48.461, 34.265).$SWIZZLE$)) * 98482.98482);",
                "return vec3(a, b, c);",
            },
    });

    static inline const std::array<CodeGen::Function, 4> rand4Funcs = CodeGen::makeGeneric({
        .id = "rand4",
        .returnType = Types::vec4,
        .params = {{"v", Types::none}},
        .body =
            {
                "float a = fract(sin(dot(v + 44.860, vec4(97.616, 88.343, 66.489, 31.076).$SWIZZLE$)) * 38604.82917);",
                "float b = fract(sin(dot(v + 40.459, vec4(98.232, 31.334, 50.115, 50.089).$SWIZZLE$)) * 66520.36050);",
                "float c = fract(sin(dot(v + 13.523, vec4(42.317, 37.769, 79.459, 43.089).$SWIZZLE$)) * 56508.99516);",
                "float d = fract(sin(dot(v + 68.554, vec4(12.138, 48.412, 56.694, 21.921).$SWIZZLE$)) * 24657.73291);",
                "return vec4(a, b, c, d);",
            },
    });

    static inline const std::array<const std::array<CodeGen::Function, 4>*, 4> randsFuncs = {
        &randFuncs,
        &rand2Funcs,
        &rand3Funcs,
        &rand4Funcs,
    };

    void evaluate(CodeGenerator& generator) override
    {
        auto input = getInput(0);
        if (const auto* t = std::get_if<GenType>(&input.type))
        {
            const auto& func = (*randsFuncs[arrity - 1])[t->arrity - 1];
            setOutput(0, generator.callFunc(func, {input}));
        }
    }

    void serialize(Serializer& s) override
    {
        ExpressionNode::serialize(s);
    }

    static void registerArchetypes(ArchetypeRepo& repo)
    {
        repo.add<RandomNode>({"Utils", "random", "Random", {{"seed", Types::none}}, {{"", Types::scalar}}}, 1);
        repo.add<RandomNode>({"Utils", "random2", "Random 2D", {{"seed", Types::none}}, {{"", Types::vec2}}}, 2);
        repo.add<RandomNode>({"Utils", "random3", "Random 3D", {{"seed", Types::none}}, {{"", Types::vec3}}}, 3);
        repo.add<RandomNode>({"Utils", "random4", "Random 4D", {{"seed", Types::none}}, {{"", Types::vec4}}}, 4);
    }
};