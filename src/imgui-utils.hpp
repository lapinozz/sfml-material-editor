#pragma once

struct FloatField
{
	float value{};
	float fieldWidth{};
	bool widthDirty = true;

	std::string id;
	static inline std::size_t nextId{};

	FloatField()
	{
		nextId++;
		id = "##FloatField_";
		id += std::to_string(nextId);
	}

	Value toValue() const
	{
		return Value{ makeValueType<ScalarType>(), std::format("{:1f}f", value) };
	}

	void update()
	{
		const char* format = "%g";       
		
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

		ImGui::SetNextItemWidth(fieldWidth);
		if (ImGui::InputFloat(id.c_str(), &value, 0, 0, format) || fieldWidth == 0)
		{

		}

		ImGui::PopStyleVar(2);

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const auto imId = window->GetID(id.c_str());
		ImGuiInputTextState* state = ImGui::GetInputTextState(imId);
		if (state && ImGui::GetActiveID() == imId)
		{
			state->ScrollX = 0;

			std::array<char, 64> buf{};
			ImTextStrToUtf8(buf.data(), buf.size(), &state->TextW.front(), nullptr);
			fieldWidth = ImGui::CalcTextSize(buf.data()).x + 10;

			widthDirty = true;
		}
		else if (widthDirty)
		{
			std::array<char, 64> buf{};
			const auto written = std::snprintf(buf.data(), buf.size(), format, value);
			fieldWidth = ImGui::CalcTextSize(buf.data()).x + 10;

			widthDirty = false;
		}
	}
};

void serialize(Serializer& s, FloatField& f)
{
	s.serialize(f.value);
}

void serialize(Serializer& s, ImVec2& v)
{
	s.serialize("x", v.x);
	s.serialize("y", v.y);
}