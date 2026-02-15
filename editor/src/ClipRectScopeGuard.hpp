#pragma once

class ClipRectScopeGuard
{

public:
    ClipRectScopeGuard()
    {
        const ImGuiWindow* window = ImGui::GetCurrentWindow();
        const auto clipRect = window->ClipRect;
        ImGui::PushClipRect(clipRect.Min, clipRect.Max, false);
    }

    ~ClipRectScopeGuard()
    {
        ImGui::PopClipRect();
    }
};