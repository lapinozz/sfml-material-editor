#pragma once

class ViewportScopeGuard
{
	ImVec2 OldPos;
	ImVec2 OldSize;

public:

	ViewportScopeGuard()
	{
		auto* mainViewport = ((ImGuiViewportP*)(void*)ImGui::GetMainViewport());
		OldPos = mainViewport->Pos;
		OldSize = mainViewport->Size;

		mainViewport->Pos = ImVec2(-99999999.f, -99999999.f);
		mainViewport->Size = ImVec2(999999999.f, 999999999.f);
	}

	~ViewportScopeGuard()
	{
		auto* mainViewport = ((ImGuiViewportP*)(void*)ImGui::GetMainViewport());
		mainViewport->Pos = OldPos;
		mainViewport->Size = OldSize;
	}
};