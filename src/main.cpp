#include <ranges>
#include <unordered_map>
#include <variant>
#include <array>
#include <functional>
#include <iostream>
#include <format>
#include <fstream>

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include <imgui_node_editor.h>
#include <imgui_node_editor_internal.h>

#include "ImGuiColorTextEdit/TextEditor.h"

#include "serializer.hpp"

#include "value.hpp"
#include "imgui-utils.hpp"
#include "graph.hpp"

#include "nodes/archetype.hpp"
#include "nodes/expression.hpp"
#include "code-generator.hpp"

#include "nodes/add.hpp"
#include "nodes/value.hpp"
#include "nodes/output.hpp"

namespace ed = ax::NodeEditor;

std::vector<std::unique_ptr<NodeArchetype>> archetypes;

void serialize(Serializer& s, Graph::Node::Ptr& n)
{
	if (s.isSaving)
	{
		auto& node = static_cast<ExpressionNode&>(*n);
		s.serialize("type_id", node.archetype->id);
		node.serialize(s);
	}
	else
	{
		std::string typeId;
		s.serialize("type_id", typeId);

		auto it = std::ranges::find_if(archetypes, [&](auto& arch) {
			return arch->id == typeId;
		});

		n = (*it)->createNode();

		auto& node = static_cast<ExpressionNode&>(*n);
		node.serialize(s);
	}
}

int main()
{
	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;
	sf::RenderWindow window{ sf::VideoMode{ { 1920u / 2u, 1080u / 2u } }, "CMake SFML Project", sf::Style::Default, sf::State::Windowed, settings };
    window.setVerticalSyncEnabled(true);
	(void)ImGui::SFML::Init(window);

	ImGui::StyleColorsDark();

	ed::Config config;
	config.SettingsFile = "Simple.json";
	auto m_Context = ed::CreateEditor(&config);

	archetypes.push_back(NodeArchetype::makeArchetype<ValueNode>({
		"scalar",
		"Scalar",
		{},
		{
			{ "", makeValueType<ScalarType>()}
		}
	}));

	archetypes.push_back(NodeArchetype::makeArchetype<AddNode>({
		"add",
		"Add",
		{
			{ "A", makeValueType<ScalarType>()},
			{ "B", makeValueType<ScalarType>()}
		},
		{
			{ "", makeValueType<ScalarType>()},
		}
		}));

	archetypes.push_back(NodeArchetype::makeArchetype<OutputNode>({
		"out",
		"Out",
		{
			{ "Frag Color", makeValueType<ScalarType>()}
		},
		{
		}
		}));

	Graph graph;

	graph.addNode(archetypes[0]->createNode());
	graph.addNode(archetypes[0]->createNode());
	graph.addNode(archetypes[0]->createNode());
	graph.addNode(archetypes[0]->createNode());
	graph.addNode(archetypes[1]->createNode());
	graph.addNode(archetypes[1]->createNode());
	graph.addNode(archetypes[2]->createNode());

	ImGuiIO& io = ImGui::GetIO(); 
	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF("./Cousine-Regular.ttf", 16);
	(void)ImGui::SFML::UpdateFontTexture();
	ImGui::SFML::GetFontTexture().setSmooth(true);

	TextEditor editor;
	auto lang = TextEditor::LanguageDefinition::GLSL();
	editor.SetLanguageDefinition(lang);
	editor.SetPalette(TextEditor::GetDarkPalette());
	editor.SetReadOnly(true); 
	
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Border] = ImVec4(0.56f, 0.56f, 0.56f, 0.50f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.67f);

    sf::Clock clock;
    while (window.isOpen())
	{
		ed::SetCurrentEditor(m_Context);

        while (const auto event = window.pollEvent()) 
		{
            ImGui::SFML::ProcessEvent(window, event);

            if (auto* e = event.getIf<sf::Event::Closed>())
            {
                window.close();
            }
			else if (auto* e = event.getIf<sf::Event::KeyPressed>())
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

					s.serialize("nodes", graph.nodes);
					s.serialize("links", graph.links);

					std::ofstream of("./test.json");
					of << j;
					of.close();
				}
			}
        }

		ImGui::SFML::Update(window, clock.restart());		
		
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		static bool isOpen = true;
		ImGui::Begin("main", &isOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

		static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;
		ImGui::BeginTable("table", 2, flags);

		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);

		ImGui::BeginChild("wow", {}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::Separator();
		ed::Begin("My Editor", ImVec2(0.0, 0.0f));

		CodeGenerator codeGen(graph);

		for (auto& node : graph.nodes)
		{
			if (!node)
			{
				continue;
			}

			static_cast<ExpressionNode&>(*node).update(graph);
		}

		for (auto& node : graph.nodes)
		{
			if (!node)
			{
				continue;
			}

			static_cast<ExpressionNode&>(*node).draw();

			if(auto* n = dynamic_cast<OutputNode*>(node.get()))
			{
				const auto value = codeGen.evaluate(n->id.makeInput(0));
				codeGen.body.push_back("gl_FrontColor = " + value.code + ";");
			}
		}

		editor.SetText(codeGen.finalize());

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
							// Since we accepted new link, lets add one to our list of links.
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

		for (auto& link : graph.links)
			ed::Link(link, link.from(), link.to());

		ed::End();
		ImGui::Separator();
		ed::SetCurrentEditor(nullptr);
		ImGui::EndChild();

		ImGui::TableSetColumnIndex(1);

		ImGui::BeginChild("wow2");
		editor.Render("TextEditor");
		ImGui::EndChild();

		ImGui::EndTable();

		ImGui::End();
		ImGui::PopStyleVar(1);

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}
