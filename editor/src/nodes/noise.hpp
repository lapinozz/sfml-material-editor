#pragma once

#include "archetypes.hpp"
#include "code-generator.hpp"
#include "expression.hpp"
#include "noise_funcs.hpp"
#include "random.hpp"
#include "value.hpp"

struct NoiseNode : ExpressionNode
{
    using ExpressionNode::ExpressionNode;

    void update(GraphContext* inGraphContext) override
    {
        ExpressionNode::update(inGraphContext);
    }

    void evaluate(CodeGenerator& generator) override
    {
        auto seedInput = getInput(0);
        auto posInput = getInput(1);
        auto periodInput = getInput(2);
        auto octaveInput = getInput(3);
        auto octaveMaskInput = getInput(4);
        if (posInput && posInput && periodInput && octaveInput)
        {
            generator.addFunc(RandomNode::randFuncs[0]);
            generator.addFunc(NoiseNode_funcs::mod289[3]);
            generator.addFunc(NoiseNode_funcs::permute[3]);
            generator.addFunc(NoiseNode_funcs::taylorInvSqrt[3]);

            if (posInput.type == Types::vec2)
            {
                generator.addFunc(NoiseNode_funcs::fade[1]);
                generator.addFunc(NoiseNode_funcs::safeMod_vec4);
                generator.addFunc(NoiseNode_funcs::perlin2d);

                const auto& func = NoiseNode_funcs::perlin2dLoop;
                setOutput(0, generator.callFunc(func, {seedInput, posInput, periodInput, octaveInput, octaveMaskInput}));
            }
            else if (posInput.type == Types::vec3)
            {
                generator.addFunc(NoiseNode_funcs::fade[2]);
                generator.addFunc(NoiseNode_funcs::mod289[2]);
                generator.addFunc(NoiseNode_funcs::safeMod_vec3);
                generator.addFunc(NoiseNode_funcs::perlin3d);

                const auto& func = NoiseNode_funcs::perlin3dLoop;
                setOutput(0, generator.callFunc(func, {seedInput, posInput, periodInput, octaveInput, octaveMaskInput}));
            }
            else if (posInput.type == Types::vec4)
            {
                generator.addFunc(NoiseNode_funcs::fade[3]);
                generator.addFunc(NoiseNode_funcs::safeMod_vec4);
                generator.addFunc(NoiseNode_funcs::perlin4d);

                const auto& func = NoiseNode_funcs::perlin4dLoop;
                setOutput(0, generator.callFunc(func, {seedInput, posInput, periodInput, octaveInput, octaveMaskInput}));
            }
        }
    }

    void serialize(Serializer& s) override
    {
        ExpressionNode::serialize(s);
    }

    static void registerArchetypes(ArchetypeRepo& repo)
    {
        repo.add<NoiseNode>(
            {"Utils",
             "noise",
             "Noise",
             {
                 {"seed", Types::scalar},
                 {"position", Types::none},
                 {"period", Types::none},
                 {"octaves", Types::none},
                 {"octaves mask", Types::none},
             },
             {
                 {"", Types::scalar},
             },
             {
                 {{Types::scalar, Types::vec2, Types::vec2, Types::scalar, Types::vec2}, {Types::scalar}},
                 {{Types::scalar, Types::vec3, Types::vec3, Types::scalar, Types::vec3}, {Types::scalar}},
                 {{Types::scalar, Types::vec4, Types::vec4, Types::scalar, Types::vec4}, {Types::scalar}},
             }});
    }
};