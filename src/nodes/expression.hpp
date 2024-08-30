#pragma once

#include "../constants.hpp"

struct CodeGenerator;

struct ExpressionNode : Graph::Node
{
	struct NodeArchetype* archetype;

	struct Input : public NodeArchetype::Input
	{
		std::string error;
		Value value;

		bool hasError() const
		{
			return error.size() > 0;
		}
	};

	struct Output : public NodeArchetype::Output
	{
		Value value;
	};

	std::vector<Input> inputs;
	std::vector<Output> outputs;

	using Ptr = std::unique_ptr<ExpressionNode>;

	ExpressionNode(struct NodeArchetype* archetype) : archetype{ archetype }, inputs{ archetype->inputs.begin(), archetype->inputs.end() }, outputs{archetype->outputs.begin(), archetype->outputs.end() }
	{

	}

	Value getInput(uint8_t index) const
	{
		return inputs.at(index).value;
	}

	void setOutput(uint8_t index, const Value& value)
	{
		outputs.at(index).value = value;
	}

	void setOutput(uint8_t index, Value&& value)
	{
		outputs.at(index).value = std::move(value);
	}

	virtual void serialize(Serializer& s)
	{
		s.serialize("id", id);

		using namespace std::placeholders;
		s.at("pos").serializeValue<ImVec2>(std::bind(ed::GetNodePosition, id), std::bind(ed::SetNodePosition, id, _1));
		//ed::GetNodePosition()
	}

	virtual void update(const Graph& graph)
	{
		for (std::size_t x{}; x < inputs.size(); x++)
		{
			inputs[x].link = graph.findLink(id.makeInput(x));
			inputs[x].value = {};
			inputs[x].error = {};
		}
		for (std::size_t x{}; x < outputs.size(); x++)
		{
			outputs[x].link = graph.findLink(id.makeOutput(x));
			outputs[x].value = {};
		}
	}

	virtual void evaluate(CodeGenerator& generator) = 0;

	virtual void draw()
	{
		ImGui::PushID(id.Get());

		ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(8, 4, 8, 8));

		ed::BeginNode(id);

		ImGui::BeginVertical("node");

		drawHeader();
		const auto HeaderMin = ImGui::GetItemRectMin();
		const auto HeaderMax = ImGui::GetItemRectMax();

		ImGui::BeginHorizontal("content");

			ImGui::BeginVertical("inputs");
				drawInputPins();
				ImGui::Spring(1, 0);
			ImGui::EndVertical();

			ImGui::Spring(1, 0);

			ImGui::BeginVertical("middle", ImVec2(0, 0), 0.5);
				drawMiddle();
				ImGui::Spring(1, 0);
			ImGui::EndVertical();

			ImGui::Spring(1, 0);

			ImGui::BeginVertical("outputs", ImVec2(0, 0), 1);
				drawOutputPins();
				ImGui::Spring(1, 0);
			ImGui::EndVertical();

		ImGui::EndHorizontal();

		ImGui::EndVertical();

		ed::EndNode();

		if (ImGui::IsItemVisible())
		{
			auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);

			auto drawList = ed::GetNodeBackgroundDrawList(id);

			const auto halfBorderWidth = ed::GetStyle().NodeBorderWidth * 0.5f;

			auto HeaderColor = IM_COL32(255 / 3, 255 / 3, 0, 0);

			auto headerColor = IM_COL32(0, 0, 0, alpha) | (HeaderColor & IM_COL32(255, 255, 255, 0));

			if ((HeaderMax.x > HeaderMin.x) && (HeaderMax.y > HeaderMin.y))
			{
				const auto uv = ImVec2(0, 0);

				drawList->AddImageRounded(0,
					HeaderMin - ImVec2(8 - halfBorderWidth - 0.2, 4 - halfBorderWidth - 0.2),
					HeaderMax + ImVec2(8 - halfBorderWidth - 0.2, 0.2),
					ImVec2(0.0f, 0.0f), uv,
					headerColor, ed::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);


				drawList->AddLine(
					ImVec2(HeaderMin.x - (8 - halfBorderWidth) + 0, HeaderMax.y + 0.35),
					ImVec2(HeaderMax.x + (8 - halfBorderWidth) - 1, HeaderMax.y + 0.35),
					ImGui::ColorConvertFloat4ToU32(ed::GetStyle().Colors[ax::NodeEditor::StyleColor_NodeBorder]), ed::GetStyle().NodeBorderWidth);
			}
		}

		ed::PopStyleVar(0);

		ImGui::PopID();
	}

	virtual void drawHeader()
	{
		ImGui::BeginHorizontal("title");
		ImGui::BeginVertical("title_");
		ImGui::Text(archetype->title.c_str());
		ImGui::Dummy({ 0, -2 });
		ImGui::EndVertical();
		ImGui::EndHorizontal();
	};

	virtual void drawInputPins()
	{
		for (std::size_t x{}; x < inputs.size(); x++)
		{
			const auto& input = inputs[x];
			const auto pinId = id.makeInput(x);
			ed::BeginPin(pinId, ed::PinKind::Input);
			drawInputPin(input, pinId);
			ed::EndPin();
		}
	};

	virtual void drawInputPin(const Input& input, PinId id)
	{
		ed::PinPivotAlignment(ImVec2(0.f, 0.5f));
		ed::PinPivotSize(ImVec2(0, 0));

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 p = ImGui::GetCursorScreenPos();
		int size = 5;
		auto textSize = ImGui::CalcTextSize("In");

		const auto hasError = input.error.size() > 0;

		if (input.link)
		{
			draw_list->AddCircleFilled(ImVec2(p.x + size, p.y + textSize.y / 2), size, hasError ? LinkColorError : LinkColor);
		}

		draw_list->AddCircle(ImVec2(p.x + size, p.y + textSize.y / 2), size, input.type.toColor(), 0.f, 2.f);
		ImGui::Dummy(ImVec2(size, size));

		ImGui::SameLine();
		ImGui::Text(input.name.c_str());
	};

	virtual void drawOutputPins()
	{
		for (std::size_t x{}; x < outputs.size(); x++)
		{
			//ImGui::Spring(0, 0);
			const auto& output = outputs[x];
			const auto pinId = id.makeOutput(x);
			ed::BeginPin(pinId, ed::PinKind::Output);
			drawOutputPin(output, pinId);
			ed::EndPin();

		}
	};

	virtual void drawOutputPin(const Output& output, PinId id)
	{
		ed::PinPivotAlignment(ImVec2(1.f, 0.5f));
		ed::PinPivotSize(ImVec2(0, 0));

		ImGui::Text(output.name.c_str());
		ImGui::SameLine();
		int size = 5;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 p = ImGui::GetCursorScreenPos();
		auto textSize = ImGui::CalcTextSize("In");
		const auto color = output.link ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);
		draw_list->AddCircleFilled(ImVec2(p.x + size, p.y + textSize.y / 2), size, output.type.toColor());
		ImGui::Dummy(ImVec2(size * 2, size));
	};

	virtual void drawMiddle()
	{
	};

};