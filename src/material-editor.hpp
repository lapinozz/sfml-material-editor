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

#include "nfd.h"
#include "base64.hpp"

#include "graph-editor.hpp"

#include "imgui_node_editor_internal.h"

ValueType getParameterValueType(const ParameterValue& value)
{
	if (auto* float1Value = std::get_if<float>(&value))
	{
		return Types::scalar;
	}
	else if (auto* float2Value = std::get_if<sf::Vector2f>(&value))
	{
		return Types::vec2;
	}
	else if (auto* float3Value = std::get_if<sf::Vector3f>(&value))
	{
		return Types::vec3;
	}
	else if (auto* float4Value = std::get_if<Vector4f>(&value))
	{
		return Types::vec4;
	}
	else if (auto* textureValue = std::get_if<sf::Texture*>(&value))
	{
		return makeValueType<SamplerType>();
	}

	assert(false);
}

struct MaterialEditor
{
	sf::RenderWindow window;

	Graph graph;
	ArchetypeRepo archetypes;
	GraphContext graphContext;

	GraphEditor graphEditor{ graph, archetypes, graphContext};

	bool isMaterialDirty{};

	MaterialRepo materialRepo;
	MaterialTemplate materialTemplate;
	Material::Ptr materialInstance;

	//TextureReferences textureReferences;
	std::unordered_map<std::string, std::string> parameterToTextureReference;
	std::unordered_map<std::string, TextureReference> textureReferences;

	std::unordered_map<std::string, nlohmann::json> materialsData;

	std::vector<std::string> openTabs;
	std::string selectedMaterialTab;

	MapListBoxData materialsListBox;
	MapListBoxData parametersListBox;
	MapListBoxData texturesListBox;

	TextEditor vertexEditor;
	TextEditor fragmentEditor;

	ed::EditorContext* edContext{};

	sf::Clock clock;
	float runningTime{};

	Preview preview;

	std::string vertexCode;
	std::string fragmentCode;

	MaterialEditor() :
		window{ sf::VideoMode{ { 1800u, 900u } }, "CMake SFML Project", sf::Style::Default, sf::State::Windowed }
	{
		window.setVerticalSyncEnabled(true);

		materialInstance = materialTemplate.makeInstance();

		initImgui();
		initArchetypes();
	}

	void initImgui()
	{
		(void)ImGui::SFML::Init(window);

		ImGui::StyleColorsDark();

		ed::Config config;
		config.SettingsFile = "Simple.json";
		config.EnableSmoothZoom = true;
		edContext = ed::CreateEditor(&config);

		const float baseFontSize = 16.f;
		const float faFontSize = baseFontSize * 2.0f / 3.0f;

		const float sizeScalar = 1.5f;

		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();
		io.Fonts->AddFontFromFileTTF("./Cousine-Regular.ttf", baseFontSize * sizeScalar)->Scale = 1.f / sizeScalar;

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
	}

	void saveTab(const std::string& Id)
	{
		auto& j = materialsData[Id];
		Serializer s(true, j);

		NodeSerializer::repo = &archetypes;

		s.serialize("nodes", graph.nodes);
		s.serialize("links", graph.links);
		s.serialize("parameters", materialTemplate.parameters);
	}

	void loadTab(const std::string& Id)
	{
		auto& j = materialsData[Id];
		Serializer s(false, j);

		NodeSerializer::repo = &archetypes;

		graph.nodes.clear();
		graph.links.clear();

		s.serialize("nodes", graph.nodes);
		s.serialize("links", graph.links);
		s.serialize("parameters", materialTemplate.parameters);

		ShortId maxId{};
		for (const auto& node : graph.nodes)
		{
			if (!node)
			{
				continue;
			}

			maxId = std::max(maxId, node->id.Get());
		}
		graph.idPool.reset(maxId + 1);

		isMaterialDirty = true;
	}

	void onMaterialTabChange(const std::string& newId)
	{
		if (selectedMaterialTab.size() > 0)
		{
			saveTab(selectedMaterialTab);
		}

		selectedMaterialTab = newId;

		if (selectedMaterialTab.size() > 0)
		{
			loadTab(selectedMaterialTab);
		}
	}

	void onMaterialTabClose(const std::string& id)
	{
		std::erase(openTabs, id);
	}

	void addMaterialTab(const std::string& id)
	{
		std::erase(openTabs, id);
		openTabs.push_back(id);
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

					NodeSerializer::repo = &archetypes;

					s.serialize("parameterToTextureReference", parameterToTextureReference);
					s.serialize("textureReferences", textureReferences);
					s.serialize("materials", materialsData);
					s.serialize("openTabs", openTabs);

					selectedMaterialTab = "";

				}
				else if (e->code == sf::Keyboard::Key::S)
				{
					saveTab(selectedMaterialTab);

					json j;
					Serializer s(true, j);

					NodeSerializer::repo = &archetypes;

					s.serialize("parameterToTextureReference", parameterToTextureReference);
					s.serialize("textureReferences", textureReferences);
					s.serialize("materials", materialsData);
					s.serialize("openTabs", openTabs);

					std::ofstream of("./test.json");
					of << j;
					of.close();
				}
				else if (e->code == sf::Keyboard::Key::Y)
				{
					UEGraphAdapter::CurrentGraph = &graph;

					//FFormatter formatter;
					//formatter.Format();

					Formatter formatter{ &graph };
					formatter.format();
				}
				else if (e->code == sf::Keyboard::Key::C && e->control)
				{
					copyToClipboard();
				}
				else if (e->code == sf::Keyboard::Key::V && e->control)
				{
					pasteFromClipboard();
				}
			}
		}
	}

	void drawNodeEditor()
	{
		CodeGenerator vertexGen(graph, CodeGenerator::Type::Vertex);
		CodeGenerator fragmentGen(graph, CodeGenerator::Type::Fragment);

		graphContext.parameterTypes.clear();
		for (const auto& [id, parameter]: materialTemplate.parameters)
		{
			graphContext.parameterTypes[id] = getParameterValueType(parameter.defaultValue);
		}

		for (auto& node : graph.nodes)
		{
			if (!node)
			{
				continue;
			}

			static_cast<ExpressionNode&>(*node).update(&graphContext);
		}

		for (auto& link : graph.links)
		{
			graph.getNode<ExpressionNode>(link.to().nodeId()).inputs[link.to().index()].link = link;
			graph.getNode<ExpressionNode>(link.from().nodeId()).outputs[link.from().index()].linkCount++;
		}

		for (auto& node : graph.nodes)
		{
			if (!node)
			{
				continue;
			}

			if (auto* n = dynamic_cast<OutputNode*>(node.get()))
			{
				if (n->type == CodeGenerator::Type::Vertex)
				{
					vertexGen.evaluate(*n);
				}
				else if (n->type == CodeGenerator::Type::Fragment)
				{
					fragmentGen.evaluate(*n);
				}
			}
		}

		auto newVertexCode = vertexGen.finalize();
		auto newFragmentCode = fragmentGen.finalize();
		if (vertexCode != newVertexCode || fragmentCode != newFragmentCode)
		{
			vertexCode = std::move(newVertexCode);
			fragmentCode = std::move(newFragmentCode);

			vertexEditor.SetText(vertexCode);
			fragmentEditor.SetText(fragmentCode);

			isMaterialDirty = true;
		}

		if (isMaterialDirty)
		{
			isMaterialDirty = false;

			materialRepo.ownedTextures.clear();
			for (const auto& pair : parameterToTextureReference)
			{
				auto it = textureReferences.find(pair.second);
				if (it != textureReferences.end())
				{
					auto texture = std::make_unique<sf::Texture>(defaultTextureLoader(it->second));
					materialInstance->setValue(pair.first, texture.get());
					texture.release();
				}
			}

			materialTemplate.setSource(vertexCode, fragmentCode);
		}

		materialInstance->setValue("time", runningTime);

		graphEditor.draw();
	}

	void drawMaterialList()
	{
		auto& parameters = materialTemplate.parameters;
		if (mapListBox("Materials", materialsListBox, materialsData))
		{
			isMaterialDirty = true;

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

		if (materialsData.contains(selectedId))
		{

		}
	}

	void drawParameterEditor()
	{
		parametersListBox.draggableId = "ParameterDrag";

		auto& parameters = materialTemplate.parameters;
		if (mapListBox("Parameters", parametersListBox, parameters))
		{
			isMaterialDirty = true;

			if (parametersListBox.removed)
			{
				parameterToTextureReference.erase(*parametersListBox.removed);
			}
			else if (parametersListBox.renamed)
			{
				parameterToTextureReference[parametersListBox.renamed->second] = parameterToTextureReference[parametersListBox.renamed->first];
				parameterToTextureReference.erase(parametersListBox.renamed->first);
			}
		}

		const auto& selectedId = parametersListBox.selectedId;

		if (parameters.contains(selectedId))
		{
			auto& parameter = parameters[selectedId];

			const auto getFloatSpan = [](auto& variant) -> std::span<float>
			{
				if (auto* float1Value = std::get_if<float>(&variant))
				{
					return { float1Value, 1 };
				}
				else if (auto* float2Value = std::get_if<sf::Vector2f>(&variant))
				{
					return { &float2Value->x, 2 };
				}
				else if (auto* float3Value = std::get_if<sf::Vector3f>(&variant))
				{
					return { &float3Value->x, 3 };
				}
				else if (auto* float4Value = std::get_if<Vector4f>(&variant))
				{
					return { &float4Value->x, 4 };
				}

				return {};
			};

			const auto itemLabels = "Float\0Vec2\0Vec3\0Vec4\0Texture\0";
			int typeIndex = parameter.defaultValue.index();
			if (ImGui::Combo("Type", &typeIndex, itemLabels))
			{
				Vector4f oldValues{};
				if (auto floatSpan = getFloatSpan(parameter.defaultValue); floatSpan.size() > 0)
				{
					std::copy(floatSpan.begin(), floatSpan.end(), &oldValues.x);
				}

				variantEmplace(parameter.defaultValue, typeIndex);

				if (auto floatSpan = getFloatSpan(parameter.defaultValue); floatSpan.size() > 0)
				{
					std::copy(&oldValues.x, &oldValues.x + floatSpan.size(),  floatSpan.data());
				}

				isMaterialDirty = true;
			}

			if (auto floatSpan = getFloatSpan(parameter.defaultValue); floatSpan.size() > 0)
			{
				if (floatSpan.size() == 1)
				{
					isMaterialDirty |= ImGui::InputFloat("Default Value", floatSpan.data());
				}
				else if (floatSpan.size() == 2)
				{
					isMaterialDirty |= ImGui::InputFloat2("Default Value", floatSpan.data());
				}
				else if (floatSpan.size() == 3)
				{
					isMaterialDirty |= ImGui::InputFloat3("Default Value", floatSpan.data());
				}
				else if (floatSpan.size() == 4)
				{
					isMaterialDirty |= ImGui::InputFloat4("Default Value", floatSpan.data());
				}
			}
			else if (auto* textureReference = std::get_if<sf::Texture*>(&parameter.defaultValue))
			{				
				std::string& currentRef = parameterToTextureReference[selectedId];
				
				if (ImGui::BeginCombo("Texture", currentRef.c_str()))
				{
					for (const auto& pair : textureReferences)
					{
						const auto& textureReference = pair.second;
						const auto& textureId = pair.first;
						const bool is_selected = textureId == currentRef;
						if (ImGui::Selectable(textureId.c_str(), is_selected))
						{
							currentRef = textureId;

							isMaterialDirty = true;
						}

						if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}
		}
	}

	void drawTexturesEditor()
	{
		static std::optional<sf::Texture> previewTexture;

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
					nfdu8filteritem_t filter{ "Image File", "bmp,png,tga,jpg,gif,psd,hdr,pic,pnm" };
					nfdopendialogu8args_t args = { 0 };
					args.filterList = &filter;
					args.filterCount = 1;
					nfdu8char_t* outPath;
					nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
					if (result == NFD_OKAY)
					{
						textureReference.data = outPath;
						textureReference.data = std::filesystem::relative(textureReference.data).string();
						reloadTexture = true;
						NFD_FreePathU8(outPath);
					}
				}
			}
			else if(textureReference.type == TextureReference::Type::Embedded)
			{
				if (ImGui::Button("Select Image"))
				{
					nfdu8filteritem_t filter{ "Image File", "bmp,png,tga,jpg,gif,psd,hdr,pic,pnm" };
					nfdopendialogu8args_t args = { 0 };
					args.filterList = &filter;
					args.filterCount = 1;
					nfdu8char_t* outPath;
					nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
					if (result == NFD_OKAY)
					{
						std::string path = outPath;
						path = std::filesystem::relative(path).string();

						std::ifstream inputFile(path, std::ios_base::binary);

						if (inputFile)
						{
							const std::string fileContent{
								(std::istreambuf_iterator<char>(inputFile)),
								std::istreambuf_iterator<char>()
							};

							textureReference.data = base64::to_base64(fileContent);
						}

						reloadTexture = true;
						NFD_FreePathU8(outPath);
					}
				}
			}

			if (reloadTexture)
			{
				isMaterialDirty = true;
				previewTexture = std::nullopt;

				if (textureReference.type == TextureReference::Type::Path)
				{
					previewTexture.emplace();
					previewTexture->loadFromFile(textureReference.data);
				}
				else if (textureReference.type == TextureReference::Type::Embedded)
				{
					previewTexture.emplace();

					const std::string textureData = base64::from_base64(textureReference.data);
					previewTexture->loadFromMemory(textureData.data(), textureData.size());
				}
			}

			if (previewTexture && previewTexture->getSize().x && previewTexture->getSize().y)
			{
				ImGuiStyle& style = ImGui::GetStyle();
				float avail = ImGui::GetContentRegionAvail().x - style.FramePadding.x * 2.0f - 100.f;
				const auto scale = std::min({ avail / previewTexture->getSize().x, avail / previewTexture->getSize().y, 1.f});

				sf::Sprite sprite(*previewTexture);
				sprite.scale({ scale, scale });
				ImGui::SetCursorPosX((avail - previewTexture->getSize().x * scale) / 2);

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
				if (ImGui::MenuItem("New")) { /* Handle New action */ }
				if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Handle Open action */ }
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
			if (ImGui::BeginTabItem("Params"))
			{
				drawParameterEditor();

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Textures"))
			{
				drawTexturesEditor();

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
		ImGui::EndChild();

		ImGui::TableSetColumnIndex(1);

		if (ImGui::BeginTabBar("middle_tabs", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable))
		{
			ImGuiTabBar* tab_bar = ImGui::GetCurrentTabBar();

			if (openTabs.empty())
			{
				openTabs.push_back("tab 1");
				openTabs.push_back("tab 2");
				openTabs.push_back("tab 3");
			}

			auto newOpenTabs = openTabs;
			newOpenTabs;


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

				const auto name = ImGui::TabBarGetTabName(tab_bar, tab);

				if (!name)
				{
					continue;
				}

				openTabs.push_back(name);
			}

			ImGui::EndTabBar();
		}


		drawNodeEditor();

		ImGui::TableSetColumnIndex(2);

		ImGui::BeginChild("right_side");
		if (ImGui::BeginTabBar("right_tabs", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Preview"))
			{
				preview.update(materialInstance->getShader());

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

			ImGui::EndTabBar();
		}
		ImGui::EndChild();

		ImGui::EndTable();

		ImGui::End();
		ImGui::PopStyleVar(1);
	}

	void copyToClipboard()
	{
		auto nodeIds = GraphUtils::getSelectedNodes();
		if (nodeIds.size() == 0)
		{
			return;
		}

		std::erase_if(nodeIds, [&](auto node)
		{
			return graph.findNode<OutputNode>(node) != nullptr;
		});

		auto links = graph.links;
		std::erase_if(links, [&](auto link)
		{
			return !nodeIds.contains(link.from().nodeId()) || !nodeIds.contains(link.to().nodeId());
		});

		auto nodesToSave = nodeIds 
			| std::views::transform([&](auto node) {return graph.findNode<ExpressionNode>(node); })
			| std::ranges::to<std::vector>();

		json j;
		Serializer s(true, j);

		NodeSerializer::repo = &archetypes;

		s.serialize("nodes", nodesToSave);
		s.serialize("links", links);

		j["type"] = "MLS-subgraph";

		std::string result = j.dump(4);
		sf::Clipboard::setString(result);
	}

	void pasteFromClipboard()
	{	
		json j = json::parse(sf::Clipboard::getString().toAnsiString());
		if (j["type"] != "MLS-subgraph")
		{
			return;
		}

		std::unordered_map<NodeId, NodeId> newIdMap;
		for (auto& node : j["nodes"])
		{
			const auto oldId = node["id"].template get<NodeId::InnerType>();
			const auto newId = graph.idPool.take();
			newIdMap.emplace(oldId, newId);
			node["id"] = newId;
		}

		Serializer s(false, j);
		NodeSerializer::repo = &archetypes;

		std::vector<LinkId> links;
		std::vector<Graph::Node::Ptr> nodes;

		s.serialize("nodes", nodes);
		s.serialize("links", links);

		sf::Vector2f boundsMin;
		sf::Vector2f boundsMax;

		for (const auto& node : nodes)
		{
			const auto pos = ed::GetNodePosition(node->id);

			boundsMin.x = std::min(boundsMin.x, pos.x);
			boundsMin.y = std::min(boundsMin.y, pos.y);

			boundsMax.x = std::max(boundsMax.x, pos.x);
			boundsMax.y = std::max(boundsMax.y, pos.y);
		}

		const auto viewRect = reinterpret_cast<ax::NodeEditor::Detail::EditorContext*>(ed::GetCurrentEditor())->GetViewRect();
		const auto center = (viewRect.Min + viewRect.Max) / 2;
		
		const auto offset = center - (boundsMin + boundsMax) / 2.f;

		ed::ClearSelection();
		for (auto& node : nodes)
		{
			const auto id = node->id;
			ed::SelectNode(id, true);
			ed::SetNodePosition(id, ed::GetNodePosition(id) + offset);
			graph.AddNode(std::move(node));
		}

		for (auto& link : links)
		{
			PinId from = link.from();
			PinId to = link.to();

			to = PinId::makeInput(newIdMap.at(to.nodeId()), to.index());
			from = PinId::makeOutput(newIdMap.at(from.nodeId()), from.index());

			graph.addLink(from, to);
		}
	}

	void update()
	{
		while (window.isOpen())
		{
			ed::SetCurrentEditor(edContext);

			processEvents();

			const auto deltaTiem = clock.restart();
			runningTime += deltaTiem.asSeconds();
			ImGui::SFML::Update(window, deltaTiem);

			bool vertexOutFound = false;
			bool fragmentOutFound = false;

			for (auto& node : graph.nodes)
			{
				if (!node)
				{
					continue;
				}

				if (auto* n = dynamic_cast<OutputNode*>(node.get()))
				{
					if (n->type == CodeGenerator::Type::Vertex)
					{
						vertexOutFound = true;
					}
					else if (n->type == CodeGenerator::Type::Fragment)
					{
						fragmentOutFound = true;
					}
				}
			}

			if (!vertexOutFound)
			{
				auto node = graph.AddNode(archetypes.get("out_vertex").createNode());
				ed::SetNodePosition(node.id, ImVec2(0.f, -100.f));
			}

			if (!fragmentOutFound)
			{
				auto node = graph.AddNode(archetypes.get("out_fragment").createNode());
				ed::SetNodePosition(node.id, ImVec2(0.f, 100.f));
			}

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