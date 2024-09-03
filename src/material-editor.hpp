#pragma once

#include "misc/cpp/imgui_stdlib.h"

#include "inlineFonts/includes/IconsFontAwesome6.h"
#include "inlineFonts/includes/IconsFontAwesome6.h_fa-solid-900.ttf.h"

#include "preview.hpp"
#include "constants.hpp"

struct MaterialEditor
{
	sf::RenderWindow window;

	ArchetypeRepo archetypes;
	Graph graph;

	TextEditor vertexEditor;
	TextEditor fragmentEditor;
	
	ed::EditorContext* edContext{};
	sf::Shader shader;

	sf::Clock clock;
	float runningTime{};

	Preview preview;

	std::string vertexCode;
	std::string fragmentCode;

	PinId newNodeTargetPin{0};

	MaterialEditor() :
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

		ed::Config config;
		config.SettingsFile = "Simple.json";
		config.EnableSmoothZoom = true;
		edContext = ed::CreateEditor(&config);

		const float baseFontSize = 16.f;
		const float faFontSize = baseFontSize * 2.0f / 3.0f;

		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();
		io.Fonts->AddFontFromFileTTF("./Cousine-Regular.ttf", baseFontSize);

		ImFontConfig font_cfg;
		font_cfg.FontDataOwnedByAtlas = false; 
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
		ImFontConfig icons_config;
		font_cfg.MergeMode = true;
		font_cfg.PixelSnapH = true;
		font_cfg.GlyphMinAdvanceX = faFontSize;
		io.Fonts->AddFontFromMemoryTTF((void*)s_fa_solid_900_ttf, sizeof(s_fa_solid_900_ttf), faFontSize, &font_cfg, icons_ranges);

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
				if (e->code == sf::Keyboard::Key::L)
				{
					graph.nodes.clear();
					graph.links.clear();

					json j;
					std::ifstream file("./test.json");
					file >> j;
					file.close();

					Serializer s(false, j);

					NodeSerializer::repo = &archetypes;

					s.serialize("nodes", graph.nodes);
					s.serialize("links", graph.links);

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
				}
				else if (e->code == sf::Keyboard::Key::S)
				{
					json j;
					Serializer s(true, j);

					NodeSerializer::repo = &archetypes;

					s.serialize("nodes", graph.nodes);
					s.serialize("links", graph.links);

					std::ofstream of("./test.json");
					of << j;
					of.close();
				}
			}
		}
	}

	void drawNodeEditor()
	{
		const auto mousePos = ImGui::GetMousePos();

		ImGui::BeginChild("wow", {}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::Separator();
		ed::Begin("My Editor", ImVec2(0.0, 0.0f));
		auto cursorTopLeft = ImGui::GetCursorScreenPos();

		CodeGenerator vertexGen(graph, CodeGenerator::Type::Vertex);
		CodeGenerator fragmentGen(graph, CodeGenerator::Type::Fragment);

		for (auto& node : graph.nodes)
		{
			if (!node)
			{
				continue;
			}

			static_cast<ExpressionNode&>(*node).update(graph);
		}

		for (auto& link : graph.links)
		{
			graph.findNode<ExpressionNode>(link.to().nodeId()).inputs[link.to().index()].link = link;
			graph.findNode< ExpressionNode>(link.from().nodeId()).outputs[link.from().index()].linkCount++;
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

		for (auto& node : graph.nodes)
		{
			if (!node)
			{
				continue;
			}

			static_cast<ExpressionNode&>(*node).draw();
		}

		auto newVertexCode = vertexGen.finalize();
		auto newFragmentCode = fragmentGen.finalize();
		if (vertexCode != newVertexCode || fragmentCode != newFragmentCode)
		{
			vertexCode = std::move(newVertexCode);
			fragmentCode = std::move(newFragmentCode);

			vertexEditor.SetText(vertexCode);
			fragmentEditor.SetText(fragmentCode);

			(void)shader.loadFromMemory(vertexCode, fragmentCode);
		}

		shader.setUniform("time", runningTime);

		if (ed::BeginCreate())
		{
			ed::PinId inputPinId, outputPinId;
			if (ed::QueryNewLink(&inputPinId, &outputPinId, ImVec4(255, 0, 0, 255)))
			{
				if (inputPinId && outputPinId)
				{
					const auto valid = graph.canAddLink(inputPinId, outputPinId);
					if (ed::AcceptNewItem(valid ? ImVec4(255, 255, 255, 255) : ImVec4(255, 0, 0, 255)))
					{
						if (valid)
						{
							const auto oldLink = PinId{ outputPinId }.direction() == PinDirection::In ? graph.findLink(outputPinId) : graph.findLink(inputPinId);
							if (oldLink)
							{
								graph.removeLink(oldLink);
							}

							graph.addLink(inputPinId, outputPinId);
						}
						else
						{
							ed::RejectNewItem(ImVec4(0, 255, 0, 255));
						}
					}
					else
					{
					}

				}
			}

			ed::PinId pinId = 0;
			if (ed::QueryNewNode(&pinId))
			{
				if (ed::AcceptNewItem())
				{
					newNodeTargetPin = pinId;

					ed::Suspend();
					ImGui::OpenPopup("Create New Node");
					ed::Resume();
				}
			}
		}
		ed::EndCreate(); // Wraps up object creation action handling.        

		// Handle deletion action
		if (ed::BeginDelete())
		{
			// There may be many links marked for deletion, let's loop over them.
			ed::LinkId linkId;
			while (ed::QueryDeletedLink(&linkId))
			{
				// If you agree that link can be deleted, accept deletion.
				if (ed::AcceptDeletedItem())
				{
					graph.removeLink(linkId);
				}

				// You may reject link deletion by calling:
				// ed::RejectDeletedItem();
			}

			ed::NodeId nodeId;
			while (ed::QueryDeletedNode(&nodeId))
			{
				graph.removeNode(nodeId);
			}
		}
		ed::EndDelete(); // Wrap up deletion action

		if (const LinkId link = ed::GetDoubleClickedLink())
		{
			auto& node = static_cast<ExpressionNode&>(graph.AddNode(archetypes.archetypes["bridge"].createNode()));
			ed::SetNodePosition(node.id, ed::ScreenToCanvas(mousePos));

			graph.addLink(link.from(), node.id.makeInput(0));
			graph.addLink(node.id.makeOutput(0), link.to());

			graph.removeLink(link);
		}

		//ed::PushStyleVar(ax::NodeEditor::StyleVar_LinkStrength, 10.f);

		for (auto& link : graph.links)
		{
			const auto from = link.from();
			const auto to = link.to();
			const auto& node = graph.findNode<ExpressionNode>(to.nodeId());
			const auto& input = node.inputs[to.index()];
			const auto color = input.hasError() ? LinkColorError : LinkColor;

			ed::Link(link, from, to, color, 2.f);
		}

		std::string error{};
		if(const LinkId link = ed::GetHoveredLink())
		{
			const auto to = link.to();
			const auto& node = graph.findNode<ExpressionNode>(to.nodeId());
			const auto& input = node.inputs[to.index()];
			if (input.hasError())
			{
				error = input.error;
			}
		}
		else if (const PinId pin = ed::GetHoveredPin(); pin && pin.direction() == PinDirection::In)
		{
			const auto& node = graph.findNode<ExpressionNode>(pin.nodeId());
			const auto& input = node.inputs[pin.index()];
			if (input.hasError())
			{
				error = input.error;
			}
		}

		if (!error.empty())
		{
			ed::Suspend();
			if (ImGui::BeginTooltip())
			{
				ImGui::Text(error.c_str());
				ImGui::EndTooltip();
			}
			ed::Resume();
		}

		ImGui::SetCursorScreenPos(cursorTopLeft);

		auto openPopupPosition = ImGui::GetMousePos();
		ed::Suspend();
		if (ed::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Create New Node");
		}
		ed::Resume();

		ed::Suspend(); 
		ImGui::SetNextWindowSize({300.f, 400.f});
		if (ImGui::BeginPopup("Create New Node"))
		{
			ImGui::Dummy({ 200.f, 0.f });

			auto newNodePostion = ed::ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());
			//ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());
			NodeId newNode{ 0 };

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 3));

			static std::string filterStr;

			if (ImGui::IsWindowAppearing())
			{
				filterStr.clear();
				ImGui::SetKeyboardFocusHere(0);
			}

			bool openAll = ImGui::InputTextWithHint("##filter", ICON_FA_MAGNIFYING_GLASS " Search", &filterStr);

			std::unordered_map<std::string, std::vector<std::string>> categories;

			const auto isFilteredOut = [&](const auto& arch)
			{
				if(filterStr.empty())
				{
					return false;
				}

				if (arch.category.contains(filterStr))
				{
					return false;
				}

				if (arch.id.contains(filterStr))
				{
					return false;
				}

				if (arch.title.contains(filterStr))
				{
					return false;
				}

				return true;
			};

			for (const auto& [id, archetype] : archetypes.archetypes)
			{
				if (!isFilteredOut(archetype))
				{
					categories[archetype.category].push_back(archetype.id);
				}
			}

			for (auto& [category, archetypeIds] : categories)
			{
				if (category.empty())
				{
					continue;
				}

				std::ranges::sort(archetypeIds);

				if (openAll)
				{
					ImGui::SetNextItemOpen(true);
				}

				ImGui::BeginChild("options");
				if (ImGui::TreeNodeEx(category.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, category.c_str()))
				{
					for (const auto& archetypeId : archetypeIds)
					{
						const auto& archetype = archetypes.archetypes[archetypeId];
						if (ImGui::MenuItem(archetype.title.c_str()))
						{
							newNode = graph.AddNode(archetype.createNode()).id;
						}
					}

					ImGui::TreePop();
				}
				ImGui::EndChild();
			}

			ImGui::PopStyleVar();

			if (newNode)
			{
				ImGui::CloseCurrentPopup();

				ed::SetNodePosition(newNode, newNodePostion);
				if (newNodeTargetPin)
				{
					graph.addLink(newNode.makeInput(0) ,newNodeTargetPin);
				}

				newNodeTargetPin = {0};
			}

			ImGui::EndPopup();
		}
		ed::Resume();

		if (ed::IsBackgroundDoubleClicked())
		{
			ed::NavigateToContent(0.25f);
		}

		ed::End();

		ImGui::Separator();
		ed::SetCurrentEditor(nullptr);
		ImGui::EndChild();
	}

	void drawMainWindow()
	{
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		bool isOpen = true;
		ImGui::Begin("main", &isOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

		static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;
		ImGui::BeginTable("table", 2, flags);

		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);

		drawNodeEditor();

		ImGui::TableSetColumnIndex(1);

		ImGui::BeginChild("right_side");
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("right_tabs", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Preview"))
			{
				preview.update(shader);

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
				graph.AddNode(archetypes.get("out_vertex").createNode());
			}

			if (!fragmentOutFound)
			{
				graph.AddNode(archetypes.get("out_fragment").createNode());
			}

			drawMainWindow();

 			//ImGui::Begin("Dear ImGui Style Editor", nullptr);
 			//ImGui::ShowStyleEditor();
			//ImGui::End();

			window.clear();
			ImGui::SFML::Render(window);
			window.display();
		}
	}
};