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

    static inline const std::array<CodeGen::Function, 4> randFuncs = CodeGen::makeGeneric({
        .id = "rand",
        .returnType = Types::scalar,
        .params = {{"v", Types::none}},
        .body = {"return fract(sin(dot(v, vec4(12.9898, 78.233, 45.164, 94.673).$SWIZZLE$)) * 43758.5453);"},
    });

    void evaluate(CodeGenerator& generator) override
    {
        auto input = getInput(0);
        if (const auto* t = std::get_if<GenType>(&input.type))
        {
            const auto& func = randFuncs[t->arrity - 1];
            setOutput(0, generator.callFunc(func, {input}));
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