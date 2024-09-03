#pragma once

#include "../value.hpp"

struct BridgeNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	void evaluate(CodeGenerator& generator) override
	{
		setOutput(0, getInput(0));
	}

	void draw() override
	{
		ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(3, -1, 3, 3));

		ImGui::PushID(id.Get());
		ed::BeginNode(id);

		ed::BeginPin(id.makeInput(0), ed::PinKind::Input);

		ed::PinPivotAlignment(ImVec2(0.f, 0.50f));
		ed::PinPivotSize(ImVec2(0, 0));

		ed::EndPin();

		ed::BeginPin(id.makeOutput(0), ed::PinKind::Output);

		ed::PinPivotAlignment(ImVec2(0.f, 0.05f));
		ed::PinPivotSize(ImVec2(0, 0));

		ImGui::cursor
		ImGui::Dummy(ImVec2(7.5f, 7.5f));

		ed::EndPin();

		ed::EndNode();
		ImGui::PopID();

		ed::PopStyleVar(1);
	}

	static void registerArchetypes(ArchetypeRepo& repo)
	{
		repo.add<BridgeNode>({
			"",
			"bridge",
			"Bridge",
			{
				{ "", Types::none}
			},
			{
				{ "", Types::none}
			}
		});
	}
};