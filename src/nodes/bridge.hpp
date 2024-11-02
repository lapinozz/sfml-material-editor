#pragma once

#include "../value.hpp"

struct BridgeNode : ExpressionNode
{
	using ExpressionNode::ExpressionNode;

	enum class ConnectionMode {Neutral, Input, Output};
	ConnectionMode connectionMode = ConnectionMode::Neutral;;

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
		if (PinId{ ed::GetHoveredPin() } == id.makeInput(1) || PinId{ ed::GetHoveredPin() } == id.makeOutput(1))
		{
			color.Value.x *= 0.75f;
			color.Value.y *= 0.75f;
			color.Value.z *= 0.75f;
		}
		ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(cursorPos.x + 3.5f, cursorPos.y + 3.5f), size, color);

		ImGui::Dummy(ImVec2(7.5f, 7.5f));

		for (int x = 0; x < 3; x++)
		{
			if (x == 2 && connectionMode == ConnectionMode::Neutral)
			{
				ed::PushStyleVar(ax::NodeEditor::StyleVar_SourceDirection, ImVec2{ 0.f, 0.f });
				ed::PushStyleVar(ax::NodeEditor::StyleVar_TargetDirection, ImVec2{ 0.f, 0.f });
			}

			ImGui::SetCursorScreenPos(cursorPos);

			const auto pinKind = [&]{
				if (x == 0)
				{
					return ed::PinKind::Input;
				}
				else if (x == 1)
				{
					return ed::PinKind::Output;
				}
				else if (x == 2)
				{
					return connectionMode == ConnectionMode::Input ? ed::PinKind::Input : ed::PinKind::Output;
				}
			}();

			const auto pinId = [&] {
				if (x == 0)
				{
					return id.makeInput(0);
				}
				else if (x == 1)
				{
					return id.makeOutput(0);
				}
				else if (x == 2)
				{
					return id.makeOutput(1);
				}
			}();

			ed::BeginPin(pinId, pinKind);

			ed::PinRect(cursorPos, cursorPos + ImVec2{ size * 2, size * 2 } - ImVec2(0.5f, 0.5f));

			ed::EndPin();

			if (x == 2 && connectionMode == ConnectionMode::Neutral)
			{
				ed::PopStyleVar(2);
			}
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
				{ "", Types::none},
				{ "", Types::none}
			},
			{
				{ "", Types::none},
				{ "", Types::none}
			}
		});
	}
};