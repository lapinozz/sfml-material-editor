#pragma once

#include "misc/cpp/imgui_stdlib.h"

#include "inlineFonts/includes/IconsFontAwesome6.h"
#include "inlineFonts/includes/IconsFontAwesome6.h_fa-solid-900.ttf.h"

#include "preview.hpp"
#include "constants.hpp"

#include "formatting/Private/Formatter.h"
#include "formatting/Private/UEGraphAdapter.h"

#include "formatting.hpp"
#include "graph-utils.hpp"
#include "material.hpp"
#include "map-list-box.hpp"
#include "variant-emplace.hpp"
#include "base64.hpp"
#include "file-utils.hpp"

#include "material-tab.hpp"

#include "imgui_node_editor_internal.h"

#include "editor/cousine.hpp"

#include "dialog-utils.hpp"

struct ProjectEditor
{
	sf::RenderWindow window;

	std::string currentPath;

	ArchetypeRepo archetypes;

	MaterialRepo materialRepo;

	std::unordered_map<std::string, MaterialTab> materialTabs;	
	TextureReferenceMap textureReferences;

	std::vector<std::string> openTabs;
	std::string selectedMaterialTab;

	MapListBoxData materialsListBox;
	MapListBoxData texturesListBox;

	TextEditor vertexEditor;
	TextEditor fragmentEditor;

	sf::Clock clock;
	float runningTime{};

	Preview preview;

	ProjectEditor() :
		window{ sf::VideoMode{ { 1800u, 900u } }, "CMake SFML Project", sf::Style::Default, sf::State::Windowed }
	{
		window.setVerticalSyncEnabled(true);

		initImgui();
		initArchetypes();
	}

	void initImgui()
	{
		(void)ImGui::SFML::Init(window);

		ImGui::StyleColorsDark();

		const float baseFontSize = 16.f;
		const float faFontSize = baseFontSize * 2.0f / 3.0f;

		const float sizeScalar = 1.5f;

		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();
		io.Fonts->AddFontFromMemoryCompressedTTF(Cousine_compressed_data, Cousine_compressed_size, baseFontSize * sizeScalar)->Scale = 1.f / sizeScalar;
		//io.Fonts->AddFontFromFileTTF("./Cousine-Regular.ttf", baseFontSize * sizeScalar)->Scale = 1.f / sizeScalar;

		ImFontConfig font_cfg;
		font_cfg.FontDataOwnedByAtlas = false;
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
		ImFontConfig icons_config;
		font_cfg.MergeMode = true;
		font_cfg.PixelSnapH = true;
		font_cfg.GlyphMinAdvanceX = faFontSize * sizeScalar;
		io.Fonts->AddFontFromMemoryTTF((void*)s_fa_solid_900_ttf, sizeof(s_fa_solid_900_ttf), faFontSize * sizeScalar, &font_cfg, icons_ranges)->Scale = 1.f / sizeScalar;

		(void)ImGui::SFML::UpdateFontTexture();
		ImGui::SFML::GetFontTexture()->setSmooth(true);

		auto lang = TextEditor::LanguageDefinition::GLSL();

		vertexEditor.SetLanguageDefinition(lang);
		vertexEditor.SetPalette(TextEditor::GetDarkPalette());
		vertexEditor.SetReadOnly(true);

		fragmentEditor.SetLanguageDefinition(lang);
		fragmentEditor.SetPalette(TextEditor::GetDarkPalette());
		fragmentEditor.SetReadOnly(true);

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Border] = ImVec4(0.56f, 0.56f, 0.56f, 0.50f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.67f);
	}

	void initArchetypes()
	{
		VecValueNode::registerArchetypes(archetypes);
		ScalarValueNode::registerArchetypes(archetypes);
		MakeVecNode::registerArchetypes(archetypes);
		BreakVecNode::registerArchetypes(archetypes);
		InputNode::registerArchetypes(archetypes);
		OutputNode::registerArchetypes(archetypes);
		BinaryOpNode::registerArchetypes(archetypes);
		BuiltinFuncNode::registerArchetypes(archetypes);
		BridgeNode::registerArchetypes(archetypes);
		AppendNode::registerArchetypes(archetypes);
		ParameterNode::registerArchetypes(archetypes);
		SampleTextureNode::registerArchetypes(archetypes);

		NodeSerializer::repo = &archetypes;
	}

	void onMaterialTabChange(const std::string& newId)
	{
		if (selectedMaterialTab.size() > 0)
		{
			
			//saveTab(selectedMaterialTab);
		}

		selectedMaterialTab = newId;

		if (selectedMaterialTab.size() > 0)
		{
			//loadTab(selectedMaterialTab);
		}
	}

	MaterialTab* getCurrentTab()
	{
		if (selectedMaterialTab.empty())
		{
			return nullptr;
		}

		const auto it = materialTabs.find(selectedMaterialTab);
		if (it == materialTabs.end())
		{
			return nullptr;
		}

		return &it->second;
	}

	std::optional<std::string> getProjectPath() const
	{
		if (currentPath.empty())
		{
			return std::nullopt;
		}

		return currentPath;
	}

	bool isDirty()
	{
		if (const auto fileData = FileUtils::readFile(currentPath, true))
		{
			if (const auto projectData = serializeToString())
			{
				return *fileData != *projectData;
			}
		}
		
		return true;
	}

	bool newProject()
	{
		if (!close())
		{
			return false;
		}

		return true;
	}

	bool load(std::string path = {})
	{
		if (!close())
		{
			return false;
		}

		if (path.empty())
		{
			if (const auto pathOptional = FileUtils::browseFile(false, FileUtils::MlspFilter))
			{
				path = *pathOptional;
			}
			else
			{
				return false;
			}
		}

		if (path.empty())
		{
			return false;
		}

		if (auto data = FileUtils::readFile(path, true))
		{
			if (serializeFromString(*data))
			{
				currentPath = std::move(path);
				return true;
			}
		}

		return false;

		//selectedMaterialTab = "";
	}

	bool save()
	{
		if (currentPath.empty())
		{
			if (const auto pathOptional = FileUtils::browseFile(true, FileUtils::MlspFilter))
			{
				currentPath = *pathOptional;
			}
		}

		if (currentPath.empty())
		{
			return false;
		}

		if (auto data = serializeToString())
		{
			if (std::ofstream of{ currentPath })
			{
				of << *data;
				return true;
			}
		}

		return false;
	}

	bool saveAs(std::string path = {})
	{
		if (path.empty())
		{
			if (const auto pathOptional = FileUtils::browseFile(true, FileUtils::MlspFilter))
			{
				path = *pathOptional;
			}
			else
			{
				return false;
			}
		}

		if (path.empty())
		{
			return false;
		}

		currentPath = std::move(path);
		return save();
	}

	bool close()
	{
		if (isDirty())
		{
			const auto status = Dialog::Show(Dialog::Type::YesNoCancel, "Unsaved Changes", "The project has unsaved changes.\nDo you want to save?");

			if (status == Dialog::Status::Yes)
			{
				if (!save())
				{
					return false;
				}
			}
			else if (status == Dialog::Status::Cancel)
			{
				return false;
			}
		}

		clear();

		return true;
	}

	void clear()
	{
		currentPath = {};

		materialRepo = {};

		materialTabs = {};
		textureReferences = {};

		openTabs = {};
		selectedMaterialTab = {};
	}

	void serialize(Serializer& s)
	{
		NodeSerializer::repo = &archetypes;

		s.serialize("materials", materialTabs);
		s.serialize("openTabs", openTabs);
		s.serialize("textureReferences", textureReferences);

		if (!s.isSaving)
		{
			updateTextures();
		}
	}

	std::optional<std::string> serializeToString()
	{
		try
		{
			json j;
			Serializer s(true, j);
			serialize(s);
			return j.dump();
		}
		catch (...)
		{
		}

		return std::nullopt;
	}

	bool serializeFromString(const std::string& data)
	{
		try
		{
			clear();

			auto j = json::parse(data);
			Serializer s(false, j);
			serialize(s);
			return true;
		}
		catch (...)
		{
		}

		return false;
	}

	void processEvents()
	{
		while (const auto event = window.pollEvent())
		{
			ImGui::SFML::ProcessEvent(window, *event);

			if (auto* e = event->getIf<sf::Event::Closed>())
			{
				window.close();
			}
			else if (auto* e = event->getIf<sf::Event::Resized>())
			{
				auto size = static_cast<sf::Vector2f>(e->size);
				window.setView({ size / 2, size });
			}
			else if (auto* e = event->getIf<sf::Event::KeyPressed>())
			{
				if (ImGui::GetIO().WantCaptureKeyboard)
				{
					continue;
				}

				if (e->code == sf::Keyboard::Key::L)
				{
					json j;
					std::ifstream file("./test.json");
					file >> j;
					file.close();

					Serializer s(false, j);

					serialize(s);

					selectedMaterialTab = "";

				}
				else if (e->code == sf::Keyboard::Key::S)
				{
					//saveTab(selectedMaterialTab);

					json j;
					Serializer s(true, j);

					serialize(s);

					std::ofstream of("./test.json");
					of << j;
					of.close();
				}
				else if (e->code == sf::Keyboard::Key::Y)
				{
					//UEGraphAdapter::CurrentGraph = &graph;

					//FFormatter formatter;
					//formatter.Format();

					//Formatter formatter{ &graph };
					//formatter.format();
				}
				else if (e->code == sf::Keyboard::Key::C && e->control)
				{
					if (auto* tab = getCurrentTab())
					{
						sf::Clipboard::setString(tab->graphEditor.copySelectedToString());
					}
				}
				else if (e->code == sf::Keyboard::Key::V && e->control)
				{
					if (auto* tab = getCurrentTab())
					{
						tab->graphEditor.pasteFromString(sf::Clipboard::getString().toAnsiString());
					}
				}
			}
		}
	}

	void drawNodeEditor()
	{
		if (auto* tab = getCurrentTab())
		{
			tab->update(textureReferences);
			tab->draw();

			vertexEditor.SetText(tab->vertexCode);
			fragmentEditor.SetText(tab->fragmentCode);
		}
	}

	void drawMaterialList()
	{
		if (mapListBox("Materials", materialsListBox, materialTabs))
		{
			if (materialsListBox.removed)
			{
				std::erase(openTabs, *materialsListBox.removed);
			}
			else if (materialsListBox.renamed)
			{
				std::erase(openTabs, materialsListBox.renamed->first);
				openTabs.push_back(materialsListBox.renamed->second);

				if (selectedMaterialTab == materialsListBox.renamed->first)
				{
					selectedMaterialTab = materialsListBox.renamed->second;
				}
			}
			else if (materialsListBox.added)
			{
				openTabs.push_back(*materialsListBox.added);
			}
			else if (materialsListBox.opened)
			{
				openTabs.push_back(*materialsListBox.opened);
			}
		}

		const auto& selectedId = materialsListBox.selectedId;

		if (materialTabs.contains(selectedId))
		{

		}
	}

	void updateTexture(const std::string& id)
	{
		auto& textureReference = textureReferences[id];
		textureReference.preview = {};

		if (textureReference.type == TextureReference::Type::Path)
		{
			textureReference.preview.loadFromFile(textureReference.data);
		}
		else if (textureReference.type == TextureReference::Type::Embedded)
		{
			const std::string textureData = base64::from_base64(textureReference.data);
			textureReference.preview.loadFromMemory(textureData.data(), textureData.size());
		}

		for (auto& [id, tab] : materialTabs)
		{
			tab.isMaterialDirty = true;
		}
	}

	void updateTextures()
	{
		for (auto& [id, _] : textureReferences)
		{
			updateTexture(id);
		}
	}

	void drawTexturesEditor()
	{
		bool reloadTexture{};

		if (mapListBox("Parameters", texturesListBox, textureReferences))
		{
			reloadTexture = true;
		}

		const auto& selectedId = texturesListBox.selectedId;

		if (textureReferences.contains(selectedId))
		{
			auto& textureReference = textureReferences[selectedId];

			const auto itemLabels = "Id\0Path\0Embeded\0";
			if (ImGui::Combo("Type", &(int&)textureReference.type, itemLabels))
			{
				reloadTexture = true;
				textureReference.data = {};
			}

			if (textureReference.type == TextureReference::Type::Id)
			{
				textureReference.data = selectedId;
			}
			else if(textureReference.type == TextureReference::Type::Path)
			{
				const bool isValidFile = std::filesystem::exists(std::filesystem::absolute(textureReference.data));
				ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, isValidFile ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 0, 0, 255));
				reloadTexture = reloadTexture || ImGui::InputText("Source File", &textureReference.data);
				ImGui::PopStyleColor();

				ImGui::SameLine();

				if (ImGui::Button("..."))
				{
					if (auto basePath = getProjectPath())
					{
						if (const auto path = FileUtils::browseFile(false, FileUtils::defaultImageFilter))
						{
							textureReference.data = std::filesystem::relative(*path, *basePath).string();
							reloadTexture = true;
						}
					}
					else
					{
						Dialog::Show(Dialog::Type::Error, "Save Project First", "Save the project to a file to select a relative path.");
					}
				}
			}
			else if(textureReference.type == TextureReference::Type::Embedded)
			{
				if (ImGui::Button("Select Image"))
				{
					if (const auto path = FileUtils::browseFile(false, FileUtils::defaultImageFilter))
					{
						std::ifstream inputFile(*path, std::ios_base::binary);

						if (inputFile)
						{
							const std::string fileContent{
								(std::istreambuf_iterator<char>(inputFile)),
								std::istreambuf_iterator<char>()
							};

							textureReference.data = base64::to_base64(fileContent);
						}

						reloadTexture = true;
					}
				}
			}

			if (reloadTexture)
			{
				updateTexture(selectedId);
			}

			auto& previewTexture = textureReference.preview;
			if (previewTexture.getSize().x && previewTexture.getSize().y)
			{
				ImGuiStyle& style = ImGui::GetStyle();
				float avail = ImGui::GetContentRegionAvail().x - style.FramePadding.x * 2.0f - 100.f;
				const auto scale = std::min({ avail / previewTexture.getSize().x, avail / previewTexture.getSize().y, 1.f});

				sf::Sprite sprite(previewTexture);
				sprite.scale({ scale, scale });
				ImGui::SetCursorPosX((avail - previewTexture.getSize().x * scale) / 2);

				ImGui::Image(sprite);
			}
		}
	}

	void drawMainWindow()
	{
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		bool isOpen = true;
		ImGui::Begin("main", &isOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					newProject();
				}
				if (ImGui::MenuItem("Open", "Ctrl+O"))
				{
					load();
				}
				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					save();
				}
				if (ImGui::MenuItem("Save as", "Ctrl+Shift+A"))
				{
					saveAs();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Cut")) { /* Handle Cut action */ }
				if (ImGui::MenuItem("Copy")) { /* Handle Copy action */ }
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ContextMenuInBody;
		ImGui::BeginTable("table", 3, flags);

		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);

		ImGui::BeginChild("left_side");
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("left_tabs", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Materials"))
			{
				drawMaterialList();

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Textures"))
			{
				drawTexturesEditor();

				ImGui::EndTabItem();
			}
			if (auto* tab = getCurrentTab())
			{
				if (ImGui::BeginTabItem("Params"))
				{
					tab->drawParameterEditor(textureReferences);

					ImGui::EndTabItem();
				}
			}

			ImGui::EndTabBar();
		}
		ImGui::EndChild();

		ImGui::TableSetColumnIndex(1);

		if (ImGui::BeginTabBar("middle_tabs", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable))
		{
			ImGuiTabBar* tab_bar = ImGui::GetCurrentTabBar();

			for (const auto& tabId : openTabs)
			{
				bool isOpen{true};
				if (ImGui::BeginTabItem(tabId.c_str(), &isOpen))
				{
					if (selectedMaterialTab != tabId)
					{
						onMaterialTabChange(tabId);
					}

					ImGui::EndTabItem();
				}
			}

			openTabs.resize(0);

			int tabIndex{};
			while (auto tab = ImGui::TabBarFindTabByOrder(tab_bar, tabIndex++))
			{
				if (tab->WantClose)
				{
					continue;
				}

				if (tab->Offset < 0 || tab->NameOffset < 0)
				{
					continue;
				}

				const std::string name = ImGui::TabBarGetTabName(tab_bar, tab);

				if (name.empty() || !materialTabs.contains(name))
				{
					continue;
				}

				openTabs.push_back(name);
			}

			if (!std::ranges::contains(openTabs, selectedMaterialTab))
			{
				selectedMaterialTab = {};
			}

			if (selectedMaterialTab.empty() && !openTabs.empty())
			{
				selectedMaterialTab = openTabs[0];
			}

			ImGui::EndTabBar();
		}

		drawNodeEditor();

		ImGui::TableSetColumnIndex(2);

		ImGui::BeginChild("right_side");
		if (ImGui::BeginTabBar("right_tabs", tab_bar_flags))
		{
			if (auto* tab = getCurrentTab())
			{
				if (ImGui::BeginTabItem("Preview"))
				{
					preview.update(tab->materialInstance->getShader());

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Code"))
				{
					if (ImGui::BeginTabBar("code_tabs", tab_bar_flags))
					{
						if (ImGui::BeginTabItem("Vertex"))
						{
							vertexEditor.Render("vertexEditor");

							ImGui::EndTabItem();
						}

						if (ImGui::BeginTabItem("Fragment"))
						{
							fragmentEditor.Render("fragmentEditor");

							ImGui::EndTabItem();
						}

						ImGui::EndTabBar();
					}

					ImGui::EndTabItem();
				}
			}

			ImGui::EndTabBar();
		}
		ImGui::EndChild();

		ImGui::EndTable();

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void update()
	{
		while (window.isOpen())
		{
			processEvents();

			const auto deltaTime = clock.restart();
			runningTime += deltaTime.asSeconds();
			ImGui::SFML::Update(window, deltaTime);

			drawMainWindow();

			/*
			ImGui::Begin("Dear ImGui Style Editor", nullptr);
			ImGui::ShowStyleEditor();
			ImGui::End();
			*/

			window.clear();
			ImGui::SFML::Render(window);
			window.display();
		}
	}
};