#pragma once

#include "../value.hpp"

struct OutputNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	CodeGenerator::Type type;

	OutputNode(NodeArchetype* archetype, CodeGenerator::Type type) : ExpressionNode{ archetype }, type{ type }
	{

	}

	void evaluate(CodeGenerator& generator) override
	{
		assert(type == generator.type);

		if (generator.type == CodeGenerator::Type::Vertex)
		{
			auto color = getInput(0);
			auto alpha = getInput(0);

			if (!color)
			{
				color = Value{ Types::scalar, "0.5" };
			}

			if (!alpha)
			{
				alpha = Value{ Types::scalar, "1" };
			}

			color = convert(color, Types::makeVec(3));

			generator.shaderInputs.push_back("uniform float time;");

			generator.body.push_back("gl_FrontColor = vec4(" + color.code + ", " + alpha.code + ");");
			generator.body.push_back("gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;");
			generator.body.push_back("gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;");
		}
		else if (generator.type == CodeGenerator::Type::Fragment)
		{
			generator.shaderInputs.push_back("uniform sampler2D texture;");

			generator.shaderInputs.push_back("uniform float time;");

			generator.body.push_back("vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);");
			generator.body.push_back("gl_FragColor = gl_Color * pixel;");
			//generator.body.push_back("gl_FragColor = gl_Color;");
		}
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<OutputNode>({
			"",
			"out_vertex",
			"Vertex Out",
			{
				{ "Color", Types::vec4},
			},
			{
			}
		}, CodeGenerator::Type::Vertex);

		repo.add<OutputNode>({
			"",
			"out_fragment",
			"Fragment Out",
			{
				{"Color", Types::vec4}
			},
			{
			}
		}, CodeGenerator::Type::Fragment);
	}
};