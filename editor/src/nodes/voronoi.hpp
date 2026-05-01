#pragma once

#include "voronoi_funcs.hpp"
#include "archetypes.hpp"
#include "code-generator.hpp"
#include "expression.hpp"
#include "random.hpp"
#include "value.hpp"

struct VoronoiNode : ExpressionNode
{
    using ExpressionNode::ExpressionNode;

    void update(GraphContext* inGraphContext) override
    {
        ExpressionNode::update(inGraphContext);
    }

    void evaluate(CodeGenerator& generator) override
    {
        auto posInput = getInput(0);
        auto timeInput = getInput(1);
        if (posInput && timeInput)
        {
            generator.addFunc(RandomNode::randFuncs[1]);
            generator.addFunc(RandomNode::rand2Funcs[1]);

            const auto& func = VoronoiNode_funcs::voronoi;
            const auto result = generator.callFunc(func, {posInput, timeInput});

            setOutput(0, {Types::scalar, result.code + ".x"});
            setOutput(1, {Types::scalar, result.code + ".y"});
            setOutput(2, {Types::scalar, result.code + ".z"});
            
        }
    }

    void serialize(Serializer& s) override
    {
        ExpressionNode::serialize(s);
    }

    static void registerArchetypes(ArchetypeRepo& repo)
    {
        repo.add<VoronoiNode>({
            "Utils",
            "voronoi",
            "Voronoi",
            {
                {"position", Types::vec2},
                {"time", Types::scalar},
            },
            {
                {"cell ID", Types::scalar},
                {"cell distance", Types::scalar},
                {"border distance", Types::scalar},
            },
        });
    }
};