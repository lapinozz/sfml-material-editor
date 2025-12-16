#pragma once

struct MapListBoxData
{
	std::string scrollItemId;
	std::string editItemId;
	std::string selectedId;

	std::optional<std::string> newSelection;
	std::optional<std::string> opened;
	std::optional<std::string> added;
	std::optional<std::string> removed;
	std::optional<std::pair<std::string, std::string>> renamed;

	std::string draggableId;
};

template<typename T>
bool mapListBox(std::string name, MapListBoxData& data, T& map)
{
	bool hasChange = false;

	std::string& scrollItemId = data.scrollItemId;
	std::string& editItemId = data.editItemId;
	std::string& selectedId = data.selectedId;

	data.added = std::nullopt;
	data.opened = std::nullopt;
	data.removed = std::nullopt;
	data.renamed = std::nullopt;
	data.newSelection = std::nullopt;

	std::unordered_map<std::string, std::string> toCopyList;
	std::unordered_set<std::string> toRemoveList;

	const auto findNewName = [&](std::string baseName)
	{
		std::string foundName = baseName;

		int index = 1;
		while (map.contains(foundName))
		{
			foundName = std::format("{}_{}", baseName, index++);
		}

		return foundName;
	};

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::BeginChild(name.c_str(), ImVec2(0, 260), ImGuiChildFlags_Borders, ImGuiWindowFlags_MenuBar);
	ImGui::PopStyleVar();

	if (ImGui::BeginMenuBar())
	{
		float buttonWidth = ImGui::CalcTextSize("Add New").x + ImGui::GetStyle().FramePadding.x * 2.f;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonWidth);
		if (ImGui::Button("Add New"))
		{
			const auto newName = findNewName("NewItem");
			map[newName] = {};
			scrollItemId = newName;
			editItemId = newName;
			hasChange = true;

			data.added = newName;
		}

		ImGui::EndMenuBar();
	}

	for (auto& [id, value] : map)
	{
		ImGui::PushID(id.c_str());

		if (ImGui::Selectable("##header", id == selectedId))
		{
			selectedId = id;
			data.newSelection = id;
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			data.opened = id;
		}

		if (!data.draggableId.empty())
		{
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags{}))
			{
				ImGui::SetDragDropPayload(data.draggableId.c_str(), id.data(), id.size() + 1);

				ImGui::Text("%s", id.c_str());

				ImGui::EndDragDropSource();
			}
		}

		bool focusText = false;

		if (ImGui::BeginPopupContextItem("test"))
		{
			if (ImGui::Selectable("Rename"))
			{
				focusText = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Selectable("Delete"))
			{
				toRemoveList.insert(id);
				ImGui::CloseCurrentPopup();

				data.removed = id;
			}

			if (ImGui::Selectable("Duplicate"))
			{
				const auto newName = findNewName(id + "_copy");
				scrollItemId = newName;
				editItemId = newName;
				toCopyList[newName] = id;
				ImGui::CloseCurrentPopup();

				data.added = newName;
			}
			ImGui::EndPopup();
		}

		if (focusText || id == editItemId)
		{
			editItemId = "";
			ImGui::SetKeyboardFocusHere();
		}

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.f);
		std::string buffer = id;
		ImGui::InputText("##Name", &buffer);
		ImGui::PopStyleColor();

		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			if (!map.contains(buffer))
			{
				toCopyList[buffer] = id;
				scrollItemId = buffer;
				toRemoveList.insert(id);

				data.renamed = { id, buffer };
			}
		}

		if (id == scrollItemId)
		{
			ImGui::ScrollToItem();
			scrollItemId = "";
			selectedId = id;
			data.newSelection = id;
		}

		ImGui::PopID();
	}

	ImGui::EndChild();

	if (toCopyList.size() > 0 || toRemoveList.size() > 0)
	{
		hasChange = true;
	}

	if (data.added || data.removed || data.renamed || data.newSelection || data.opened)
	{
		hasChange = true;
	}

	for (const auto& toCopy : toCopyList)
	{
		map[toCopy.first] = map[toCopy.second];
	}

	for (const auto& toRemove : toRemoveList)
	{
		map.erase(toRemove);
	}

	return hasChange;
}