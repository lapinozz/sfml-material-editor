#pragma once

#include <string>
#include <array>
#include <cstdint>

#include "value.hpp"

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
		auto str = std::format("{:1f}", value);
		while (str.length() > 0 && str.back() == '0')
		{
			str.pop_back();
		}

		str += 'f';

		return Value{ Types::scalar, str };
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

struct ValueField
{
	std::array<FloatField, 4> fields;
	ValueType type;

	Value toValue() const
	{
		if (type == Types::scalar)
		{
			return fields[0].toValue();
		}
		else if (type == Types::vec2)
		{
			return { type, std::format("vec2({}, {})", fields[0].toValue().code, fields[1].toValue().code) };
		}
		else if (type == Types::vec3)
		{
			return { type, std::format("vec3({}, {}, {})", fields[0].toValue().code, fields[1].toValue().code, fields[2].toValue().code) };
		}
		else if (type == Types::vec4)
		{
			return { type, std::format("vec4({}, {}, {}, {})", fields[0].toValue().code, fields[1].toValue().code, fields[2].toValue().code, fields[3].toValue().code) };
		}

		assert(false);
	}

	void update(ValueType inType)
	{
		type = inType;
		if (!type)
		{
			return;
		}

		int fieldCount{};

		if (type == Types::scalar)
		{
			fieldCount = 1;
		}
		else if (type == Types::vec2)
		{
			fieldCount = 2;
		}
		else if (type == Types::vec3)
		{
			fieldCount = 3;
		}
		else if (type == Types::vec4)
		{
			fieldCount = 4;
		}

		assert(fieldCount >= 1 && fieldCount <= 4);

		for (int x = 0; x < fieldCount; x++)
		{
			fields[x].update();

			if (x != fieldCount - 1)
			{
				ImGui::SameLine();
			}
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