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

    void update(GraphContext* inGraphContext) override
    {
        ExpressionNode::update(inGraphContext);
    }

    static inline const std::array<CodeGenerator::Function, 4> randFuncs{
        CodeGenerator::Function{
            .id = "rand1",
            .returnType = Types::scalar,
            .params = {{"v", Types::scalar}},
            .body = {"return fract(sin(v) * 43758.5453123);"},
        },
        {
            .id = "rand2",
            .returnType = Types::scalar,
            .params = {{"v", Types::vec2}},
            .body = {"return fract(sin(dot(v.xy ,vec2(12.9898,78.233))) * 43758.5453);"},
        },
        {
            .id = "rand3",
            .returnType = Types::scalar,
            .params = {{"v", Types::vec3}},
            .body = {"return fract(sin(dot(v.xyz ,vec3(12.9898,78.233, 137.159))) * 43758.5453);"},
        },
        {
            .id = "rand4",
            .returnType = Types::scalar,
            .params = {{"v", Types::vec4}},
            .body = {"return fract(sin(dot(v.xyzw ,vec4(12.9898,78.233, 45.164, 94.673))) * 43758.5453);"},
        },
    };

    void evaluate(CodeGenerator& generator) override
    {
        auto input = getInput(0);
        if (const auto* t = std::get_if<GenType>(&input.type))
        {
            const auto& func = randFuncs[t->arrity - 1];
            generator.addFunc(func);
            setOutput(0, generator.callFunc(func.id, {input}));
        }
    }

    void serialize(Serializer& s) override
    {
        ExpressionNode::serialize(s);
    }

    static void registerArchetypes(ArchetypeRepo& repo)
    {
        repo.add<RandomNode>({"Utils", "random", "Random", {{"seed", Types::none}}, {{"", Types::scalar}}});
    }
};