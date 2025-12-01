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

	ArchetypeRepo archetypes;
	Graph graph;

	bool isMaterialDirty{};

	MaterialRepo materialRepo;
	MaterialTemplate materialTemplate;
	Material::Ptr materialInstance;

	//TextureReferences textureReferences;
	std::unordered_map<std::string, std::string> parameterToTextureReference;
	std::unordered_map<std::string, TextureReference> textureReferences;

	MapListBoxData parametersListBox;
	MapListBoxData texturesListBox;

	TextEditor vertexEditor;
	TextEditor fragmentEditor;

	GraphContext graphContext;

	ed::EditorContext* edContext{};

	sf::Clock clock;
	float runningTime{};

	Preview preview;

	std::string vertexCode;
	std::string fragmentCode;

	std::vector<PinId> newNodeTargetPins;
	bool newNodeFilterType = true;

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

					s.serialize("parameterToTextureReference", parameterToTextureReference);
					s.serialize("textureReferences", textureReferences);
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
				else if (e->code == sf::Keyboard::Key::S)
				{
					json j;
					Serializer s(true, j);

					NodeSerializer::repo = &archetypes;

					s.serialize("nodes", graph.nodes);
					s.serialize("links", graph.links);

					s.serialize("parameterToTextureReference", parameterToTextureReference);
					s.serialize("textureReferences", textureReferences);
					s.serialize("parameters", materialTemplate.parameters);

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

	bool canTarget(ValueType outType, ValueType inType)
	{
		if (canConvert(outType, inType))
		{
			return true;
		}

		if (!outType && inType.isGenType())
		{
			return true;
		}

		if (outType.isGenType() && !inType)
		{
			return true;
		}

		return false;
	}

	bool canTarget(PinId out, PinId in)
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

		if (in.direction() == PinDirection::Out)
		{
			std::swap(in, out);
		}

		const auto& outNode = graph.getNode<ExpressionNode>(out.nodeId());
		const auto& outArchetype = *outNode.archetype;
		const auto outIndex = out.index();
		const auto* outBridge = graph.findNode<BridgeNode>(out.nodeId());

		const auto& inNode = graph.getNode<ExpressionNode>(in.nodeId());
		const auto& inArchetype = *inNode.archetype;
		const auto inputIndex = in.index();
		const auto* inBridge = graph.findNode<BridgeNode>(in.nodeId());

		if (inBridge || outBridge)
		{
			return true;
		}

		if (canTarget(outNode.outputs[outIndex].type, inNode.inputs[inputIndex].type))
		{
			return true;
		}

		for (const auto& inOverload : inArchetype.overloads)
		{
			if (canTarget(outNode.outputs[outIndex].type, inOverload.inputs[inputIndex]))
			{
				return true;
			}
		}

		for (const auto& outOverload : outArchetype.overloads)
		{
			if (canTarget(outOverload.outputs[outIndex], inNode.inputs[inputIndex].type))
			{
				return true;
			}

			for (const auto& inOverload : inArchetype.overloads)
			{
				if (canTarget(outOverload.outputs[outIndex], inOverload.inputs[inputIndex]))
				{
					return true;
				}
			}
		}

		return false;
	}

	int findConnectTarget(const NodeArchetype& out, PinId in)
	{
		const auto& inNode = graph.getNode<ExpressionNode>(in.nodeId());
		const auto& inArchetype = *inNode.archetype;
		const auto inputIndex = in.index();
		const auto* inBridge = graph.findNode<BridgeNode>(in.nodeId());

		if (inBridge)
		{
			return in.nodeId().makeInput(0);
		}

		for (int outIndex{}; outIndex < out.outputs.size(); outIndex++)
		{
			if (canTarget(out.outputs[outIndex].type, inNode.inputs[inputIndex].type))
			{
				return outIndex;
			}

			for (const auto& inOverload : inArchetype.overloads)
			{
				if (canTarget(out.outputs[outIndex].type, inOverload.inputs[inputIndex]))
				{
					return outIndex;
				}
			}
		}

		for (const auto& outOverload : out.overloads)
		{
			for (int outIndex{}; outIndex < outOverload.outputs.size(); outIndex++)
			{
				if (canTarget(outOverload.outputs[outIndex], inNode.inputs[inputIndex].type))
				{
					return outIndex;
				}

				for (const auto& inOverload : inArchetype.overloads)
				{
					if (canTarget(outOverload.outputs[outIndex], inOverload.inputs[inputIndex]))
					{
						return outIndex;
					}
				}
			}
		}

		return -1;
	}

	int findConnectTarget(PinId out, const NodeArchetype& in)
	{
		const auto& outNode = graph.getNode<ExpressionNode>(out.nodeId());
		const auto& outArchetype = *outNode.archetype;
		const auto outIndex = out.index();
		const auto* outBridge = graph.findNode<BridgeNode>(out.nodeId());

		if (outBridge)
		{
			return out.nodeId().makeOutput(0);
		}

		for (int inputIndex{}; inputIndex < in.inputs.size(); inputIndex++)
		{
			if (canTarget(outNode.outputs[outIndex].type, in.inputs[inputIndex].type))
			{
				return inputIndex;
			}
		}

		for (const auto& inOverload : in.overloads)
		{
			for (int inputIndex{}; inputIndex < inOverload.inputs.size(); inputIndex++)
			{
				if (canTarget(outNode.outputs[outIndex].type, inOverload.inputs[inputIndex]))
				{
					return inputIndex;
				}
			}
		}

		for (const auto& outOverload : outArchetype.overloads)
		{
			for (int inputIndex{}; inputIndex < in.inputs.size(); inputIndex++)
			{
				if (canTarget(outOverload.outputs[outIndex], in.inputs[inputIndex].type))
				{
					return inputIndex;
				}
			}

			for (const auto& inOverload : in.overloads)
			{
				for (int inputIndex{}; inputIndex < inOverload.inputs.size(); inputIndex++)
				{
					if (canTarget(outOverload.outputs[outIndex], inOverload.inputs[inputIndex]))
					{
						return inputIndex;
					}
				}
			}
		}

		return -1;
	}

	void updateCreate()
	{
		if (ed::BeginCreate())
		{
			bool firstLink = true;

			ed::PinId edInputPinId, edOutputPinId;
			while (ed::QueryNewLink(&edInputPinId, &edOutputPinId, ImVec4(255, 255, 255, 255)))
			{
				PinId inputPinId{ edInputPinId };
				PinId outputPinId{ edOutputPinId };

				if (ImGui::GetIO().KeyCtrl)
				{
					graph.removeLinks(inputPinId);
				}

				auto* inputBridge = inputPinId ? graph.findNode<BridgeNode>(inputPinId.nodeId()) : nullptr;
				auto* outputBridge = outputPinId ? graph.findNode<BridgeNode>(outputPinId.nodeId()) : nullptr;

				if (!inputPinId || !outputPinId)
				{
					ed::RejectNewItem();
				}
				else
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

					const auto valid = canTarget(inputPinId, outputPinId);
					if(!valid)
					{
						ed::RejectNewItem(ImVec4(255, 0, 0, 255));
					}
					else if (ed::AcceptNewItem(ImVec4(255, 255, 255, 255)))
					{
						graph.addLink(inputPinId, outputPinId);
					}
				}

				firstLink = false;
			}

			ed::PinId edPinId = 0;
			while (ed::QueryNewNode(&edPinId))
			{
				PinId pinId{ edPinId };

				if (ed::AcceptNewItem(ImVec4(255, 255, 255, 255)))
				{
					if (auto* bridge = graph.findNode<BridgeNode>(pinId.nodeId()))
					{
						if (bridge->connectionMode == BridgeNode::ConnectionMode::Input)
						{
							newNodeTargetPins.push_back(pinId.nodeId().makeInput(0));
						}
						else
						{
							newNodeTargetPins.push_back(pinId.nodeId().makeOutput(0));
						}
					}
					else
					{
						newNodeTargetPins.push_back(pinId);
					}

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

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 3));

			static std::string filterStr;
			static int selectionIndex = -1;

			if (ImGui::IsWindowAppearing())
			{
				filterStr.clear();
				ImGui::SetKeyboardFocusHere(0);
				ImGui::SetScrollY(0);
			}

			bool openAll = ImGui::InputTextWithHint("##filter", ICON_FA_MAGNIFYING_GLASS " Search", &filterStr);

			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.f, 1.f, 1.f, 0.25f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

			ImGui::Checkbox("##filter_bool", &newNodeFilterType);

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();

			ImGui::SetItemTooltip("Filter by node type");

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

			const auto isNameFilteredOut = [&](const auto& arch)
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

			const auto isTypeFilteredOut = [&](const auto& arch)
			{
				if (!newNodeFilterType)
				{
					return false;
				}

				if (newNodeTargetPins.empty())
				{
					return false;
				}
				
				for (const auto targetPin : newNodeTargetPins)
				{
					if (targetPin.direction() == PinDirection::In)
					{
						if (findConnectTarget(arch, targetPin) >= 0)
						{
							return false;
						}
					}
					else
					{
						if (findConnectTarget(targetPin, arch) >= 0)
						{
							return false;
						}
					}
				}

				return true;
			};

			for (const auto& [id, archetype] : archetypes.archetypes)
			{
				if (!isNameFilteredOut(archetype) && !isTypeFilteredOut(archetype))
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

				const auto& arch = *graph.findNode<ExpressionNode>(newNode)->archetype;

				for (const auto targetPin : newNodeTargetPins)
				{
					if (targetPin.direction() == PinDirection::In)
					{
						if (auto index = findConnectTarget(arch, targetPin); index >= 0)
						{
							graph.addLink(targetPin, newNode.makeOutput(index));
						}
					}
					else
					{
						if (auto index = findConnectTarget(targetPin, arch); index >= 0)
						{
							graph.addLink(targetPin, newNode.makeInput(index));
						}
					}
				}

				newNodeTargetPins.clear();
			}

			ImGui::EndPopup();
		}
		else
		{
			newNodeTargetPins.clear();
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

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyAlt)
		{
			if (const PinId pin = ed::GetHoveredPin())
			{
				graph.removeLinks(pin);
			}
		}

		updateCreate();
		updateDelete();

		ed::End();

		ImGui::Separator();
		ImGui::EndChild();
	}

	void drawParameterEditor()
	{
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
		ImGui::Begin("main", &isOpen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

		static ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV | ImGuiTableFlags_ContextMenuInBody;
		ImGui::BeginTable("table", 3, flags);

		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);

		ImGui::BeginChild("left_side");
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("left_tabs", tab_bar_flags))
		{
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