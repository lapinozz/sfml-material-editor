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

	PinId newNodeTargetPin{ 0 };

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
		AppendNode::registerArchetypes(archetypes);
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

	void updateCreate()
	{
		if (ed::BeginCreate())
		{
			ed::PinId edInputPinId, edOutputPinId;
			if (ed::QueryNewLink(&edInputPinId, &edOutputPinId, ImVec4(255, 0, 0, 255)))
			{
				PinId inputPinId{ edInputPinId };
				PinId outputPinId{ edOutputPinId };

				auto* inputBridge = inputPinId ? graph.findNode<BridgeNode>(inputPinId.nodeId()) : nullptr;
				auto* outputBridge = outputPinId ? graph.findNode<BridgeNode>(outputPinId.nodeId()) : nullptr;

				if (inputPinId && outputPinId)
				{
					if (inputPinId == outputPinId)
					{
						if (inputBridge)
						{
							inputBridge->connectionMode = BridgeNode::ConnectionMode::Neutral;
						}
					}
					else if (inputBridge && outputBridge)
					{
						inputBridge->connectionMode = BridgeNode::ConnectionMode::Output;
						outputBridge->connectionMode = BridgeNode::ConnectionMode::Input;
					}
					else
					{
						if (inputBridge)
						{
							inputBridge->connectionMode = outputPinId.direction() == PinDirection::In ? BridgeNode::ConnectionMode::Output : BridgeNode::ConnectionMode::Input;
						}

						if (outputBridge)
						{
							outputBridge->connectionMode = inputPinId.direction() == PinDirection::In ? BridgeNode::ConnectionMode::Output : BridgeNode::ConnectionMode::Input;
						}
					}

					if (outputBridge)
					{
						outputPinId = outputBridge->connectionMode == BridgeNode::ConnectionMode::Output ? outputPinId.nodeId().makeOutput(0) : outputPinId.nodeId().makeInput(0);
					}

					if (inputBridge)
					{
						inputPinId = inputBridge->connectionMode == BridgeNode::ConnectionMode::Output ? inputPinId.nodeId().makeOutput(0) : inputPinId.nodeId().makeInput(0);
					}

					const auto canAddLink = [&](PinId in, PinId out)
						{
							if (in == out)
							{
								return false;
							}

							if (in.direction() == out.direction())
							{
								return false;
							}

							if (in.nodeId() == out.nodeId())
							{
								return false;
							}

							if (graph.hasLink({ in, out }))
							{
								return false;
							}

							return true;
						};

					const auto valid = canAddLink(inputPinId, outputPinId);
					if (ed::AcceptNewItem(valid ? ImVec4(255, 255, 255, 255) : ImVec4(255, 0, 0, 255)))
					{
						if (valid)
						{
							const auto oldLink = outputPinId.direction() == PinDirection::In ? graph.findLink(outputPinId) : graph.findLink(inputPinId);
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
		ed::EndCreate();
	}

	void updateDelete()
	{
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
	}

	void createNewNodePopup()
	{
		ed::Suspend();
		ImGui::SetNextWindowSize({ 300.f, 400.f });
		if (ImGui::BeginPopup("Create New Node"))
		{
			ImGui::Dummy({ 200.f, 0.f });

			auto newNodePostion = ed::ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());
			NodeId newNode{ 0 };

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 3));

			static std::string filterStr;
			static int selectionIndex = -1;

			if (ImGui::IsWindowAppearing())
			{
				filterStr.clear();
				ImGui::SetKeyboardFocusHere(0);
				ImGui::SetScrollY(0);
			}

			bool openAll = ImGui::InputTextWithHint("##filter", ICON_FA_MAGNIFYING_GLASS " Search", &filterStr);

			if (openAll || ImGui::IsWindowAppearing())
			{
				selectionIndex = -1;
			}

			bool selectionChanged = false;

			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			{
				openAll = true;
				selectionIndex++;
				selectionChanged = true;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			{
				openAll = true;
				selectionIndex--;
				selectionChanged = true;
			}

			const bool createAtIndex = ImGui::IsKeyPressed(ImGuiKey_Enter);

			std::unordered_map<std::string, std::vector<std::string>> categories;

			const auto isFilteredOut = [&](const auto& arch)
				{
					if (filterStr.empty())
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

			int currentIndex = 0;

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
						const auto isSelected = selectionIndex == currentIndex;

						if (isSelected)
						{
							ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.26f, 0.59f, 0.98f, 0.80f));
						}

						if (ImGui::Selectable(archetype.title.c_str(), isSelected) || (createAtIndex && isSelected))
						{
							newNode = graph.AddNode(archetype.createNode()).id;
						}

						if (isSelected && selectionChanged)
						{
							ImGui::ScrollToRect(ImGui::GetCurrentWindow(), { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() });
						}

						if (isSelected)
						{
							ImGui::PopStyleColor(1);
						}

						currentIndex++;
					}

					ImGui::TreePop();
				}
				ImGui::EndChild();
			}

			ImGui::PopStyleVar();

			if (currentIndex > 0)
			{
				selectionIndex = std::clamp(selectionIndex, 0, currentIndex - 1);
			}

			if (newNode)
			{
				ImGui::CloseCurrentPopup();

				ed::SetNodePosition(newNode, newNodePostion);
				if (newNodeTargetPin)
				{
					if (newNodeTargetPin.direction() == PinDirection::In)
					{
						graph.addLink(newNodeTargetPin, newNode.makeOutput(0));
					}
					else
					{
						graph.addLink(newNode.makeInput(0), newNodeTargetPin);
					}
				}

				newNodeTargetPin = { 0 };
			}

			ImGui::EndPopup();
		}
		else
		{
			newNodeTargetPin = { 0 };
		}
		ed::Resume();
	}

	void drawNodeEditor()
	{
		const auto mousePos = ImGui::GetMousePos();

		ImGui::BeginChild("wow", {}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::Separator();
		ed::Begin("My Editor", ImVec2(0.0, 0.0f));

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

		if (const LinkId link = ed::GetDoubleClickedLink())
		{
			auto& node = static_cast<ExpressionNode&>(graph.AddNode(archetypes.archetypes["bridge"].createNode()));
			ed::SetNodePosition(node.id, ed::ScreenToCanvas(mousePos));

			graph.addLink(link.from(), node.id.makeInput(0));
			graph.addLink(node.id.makeOutput(0), link.to());

			graph.removeLink(link);
		}

		for (auto& link : graph.links)
		{
			const auto from = link.from();
			const auto to = link.to();
			const auto& node = graph.getNode<ExpressionNode>(to.nodeId());
			const auto& input = node.inputs[to.index()];
			const auto color = input.hasError() ? LinkColorError : LinkColor;

			ed::Link(link, from, to, color, 2.f);
		}

		std::string error{};
		if (const LinkId link = ed::GetHoveredLink())
		{
			const auto to = link.to();
			const auto& node = graph.getNode<ExpressionNode>(to.nodeId());
			const auto& input = node.inputs[to.index()];
			if (input.hasError())
			{
				error = input.error;
			}
		}
		else if (const PinId pin = ed::GetHoveredPin(); pin && pin.direction() == PinDirection::In)
		{
			const auto& node = graph.getNode<ExpressionNode>(pin.nodeId());
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

		ed::Suspend();
		if (ed::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Create New Node");
		}
		ed::Resume();

		createNewNodePopup();

		if (ed::IsBackgroundDoubleClicked())
		{
			ed::NavigateToContent(0.25f);
		}

		updateCreate();
		updateDelete();

		ed::End();

		ImGui::Separator();
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

	void copyToClipboard()
	{
		auto nodeIds = GraphUtils::getSelectedNodes();
		if (nodeIds.size() == 0)
		{
			return;
		}

		std::erase_if(nodeIds, [&](auto node)
		{
			return dynamic_cast<OutputNode*>(graph.findNode<ExpressionNode>(node)) != nullptr;
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

		const auto bounds = GraphUtils::getNodesBound(nodes);
		const auto center = ed::ScreenToCanvas(ed::GetScreenSize() / 2.f);
		const auto offset = center - bounds.getCenter();
		
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