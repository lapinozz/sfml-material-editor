#include <ranges>
#include <unordered_map>
#include <variant>
#include <array>
#include <functional>
#include <iostream>
#include <format>
#include <fstream>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

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
#include "nodes/archetypes.hpp"
#include "nodes/expression.hpp"
#include "code-generator.hpp"

#include "nodes/binary-op.hpp"
#include "nodes/builtin-func.hpp"
#include "nodes/vec-value.hpp"
#include "nodes/scalar-value.hpp"
#include "nodes/make-vec.hpp"
#include "nodes/break-vec.hpp"
#include "nodes/input.hpp"
#include "nodes/output.hpp"
#include "nodes/constants.hpp"
#include "nodes/bridge.hpp"
#include "nodes/append.hpp"

#include "material-editor.hpp"

namespace ed = ax::NodeEditor;

static void serialize(Serializer& s, Graph::Node::Ptr& n)
{
	NodeSerializer::serialize(s, n);
}

int main()
{
	MaterialEditor editor{};
	while (true)
	{
		editor.update();
	}
    ImGui::SFML::Shutdown();
}
