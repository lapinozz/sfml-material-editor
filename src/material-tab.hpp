#pragma once

#include <memory>

#include "graph-editor.hpp"

using TextureReferenceMap = std::unordered_map<std::string, TextureReference>;

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

using EditorContextDestroyer = decltype([](auto* ptr) { ed::DestroyEditor(ptr); });
using EditorContextPtr = std::unique_ptr<ed::EditorContext, EditorContextDestroyer>;
EditorContextPtr makeEditorContext()
{
	ed::Config config;
	config.SettingsFile = nullptr;
	config.EnableSmoothZoom = true;

	return EditorContextPtr{ ed::CreateEditor(&config) };
};

struct MaterialTab
{
	ArchetypeRepo& archetypes = *NodeSerializer::repo;

	Graph graph;
	GraphContext graphContext;
	EditorContextPtr edContext = makeEditorContext();

	GraphEditor graphEditor{ graph, graphContext, archetypes };

	MaterialTemplate materialTemplate;
	Material::Ptr materialInstance = materialTemplate.makeInstance();

	bool isMaterialDirty{};

	std::string vertexCode;
	std::string fragmentCode;

	std::unordered_map<std::string, std::string> parameterToTextureReference;

	MapListBoxData parametersListBox;

	MaterialTab() = default;

	MaterialTab(const MaterialTab& other)
	{
		*this = other;
	}

	MaterialTab(MaterialTab&&) = delete;

	MaterialTab& operator=(const MaterialTab& other)
	{
		json j;

		Serializer saver(true, j);
		const_cast<MaterialTab&>(other).serialize(saver);

		Serializer loader(false, j);
		serialize(loader);

		return *this;
	}

	MaterialTab& operator=(MaterialTab&&) = delete;

	void serialize(Serializer s)
	{
		ed::SetCurrentEditor(edContext.get());

		s.serialize(graphEditor);
		s.serialize("parameters", materialTemplate.parameters);
		s.serialize("parameterToTextureReference", parameterToTextureReference);

		if (!s.isSaving)
		{
			isMaterialDirty = true;
		}
	}

	void drawParameterEditor(const TextureReferenceMap& textureReferences)
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
					std::copy(&oldValues.x, &oldValues.x + floatSpan.size(), floatSpan.data());
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

	void draw()
	{
		ed::SetCurrentEditor(edContext.get());

		graphEditor.draw();
	}

	void update(const TextureReferenceMap& textureReferences)
	{
		ed::SetCurrentEditor(edContext.get());

		graphContext.parameterTypes.clear();
		for (const auto& [id, parameter] : materialTemplate.parameters)
		{
			graphContext.parameterTypes[id] = getParameterValueType(parameter.defaultValue);
		}

		graphEditor.update();

		CodeGenerator vertexGen(graph, CodeGenerator::Type::Vertex);
		CodeGenerator fragmentGen(graph, CodeGenerator::Type::Fragment);

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

			isMaterialDirty = true;
		}

		if (isMaterialDirty)
		{
			isMaterialDirty = false;

			//materialRepo.ownedTextures.clear();
			for (const auto& pair : parameterToTextureReference)
			{
				const auto it = textureReferences.find(pair.second);
				if (it != textureReferences.end())
				{
					auto texture = std::make_unique<sf::Texture>(defaultTextureLoader(it->second));
					materialInstance->setValue(pair.first, texture.get());
					texture.release();
				}
			}

			materialTemplate.setSource(vertexCode, fragmentCode);
		}

		//materialInstance->setValue("time", runningTime);
	}
};

void serialize(Serializer& s, MaterialTab& tab)
{
	tab.serialize(s);
}