#pragma once

#include "archetypes.hpp"
#include "expression.hpp"

#include <array>

struct MakeVecNode : ExpressionNode
{
    using ExpressionNode::ExpressionNode;

    uint8_t arrity;

    MakeVecNode(NodeArchetype* archetype, uint8_t arrity) : ExpressionNode{archetype}, arrity{arrity}
    {
    }

    void evaluate(CodeGenerator& generator) override
    {
        bool firstOnlySet = false;
        if (!inputs[0].link)
        {
            firstOnlySet = false;
        }

        for (uint8_t x = 1; x < arrity; x++)
        {
            if (inputs[x].link)
            {
                firstOnlySet = false;
            }
        }

        Value value;

        auto firstValue = getInput(0);

        value.code = std::format("vec{}(", arrity);
        for (uint8_t x = 0; x < arrity; x++)
        {
            if (x != 0)
            {
                value.code += ", ";
            }

            value.code += (x == 0 || firstOnlySet) ? firstValue.code : getInput(x).code;
        }

        value.code += ")";
        value.type = Types::makeVec(arrity);

        setOutput(0, value);
    }

    static void registerArchetypes(ArchetypeRepo& repo)
    {
        for (uint8_t arrity = 2; arrity <= 4; arrity++)
        {
            std::vector<NodeArchetype::Input> inputs;

            for (uint8_t x = 0; x < arrity; x++)
            {
                inputs.push_back({std::string(1, "XYZA"[x]), Types::scalar});
            }

            repo.add<MakeVecNode>({"Value",
                                   "make_vec" + std::to_string(arrity),
                                   "Make Vec" + std::to_string(arrity),
                                   inputs,
                                   {{"", Types::makeVec(arrity)}}},
                                  arrity);
        }
    }
};