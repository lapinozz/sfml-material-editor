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
		ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(3, 3, 3, 3));

		ImGui::PushID(id.Get());
		ed::BeginNode(id);

		float size = 4.f;

		const auto cursorPos = ImGui::GetCursorScreenPos();

		auto color = LinkColor;
		if (PinId{ ed::GetHoveredPin() } == id.makeInput(0) || PinId{ ed::GetHoveredPin() } == id.makeOutput(0))
		{
			color.Value.x *= 0.75f;
			color.Value.y *= 0.75f;
			color.Value.z *= 0.75f;
		}
		ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(cursorPos.x + 3.5f, cursorPos.y + 3.5f), size, color);

		ImGui::Dummy(ImVec2(7.5f, 7.5f));

		for (int x = 0; x < 2; x++)
		{
			ImGui::SetCursorScreenPos(cursorPos);
			ed::BeginPin(x == 0 ? id.makeInput(0) : id.makeOutput(0), x == 0 ? ed::PinKind::Input : ed::PinKind::Output);

			ed::PinRect(cursorPos, cursorPos + ImVec2{ size * 2, size * 2 } - ImVec2(0.5f, 0.5f));

			ed::EndPin();
		}

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