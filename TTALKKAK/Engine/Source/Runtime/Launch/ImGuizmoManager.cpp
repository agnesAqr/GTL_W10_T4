#include "ImGuizmoManager.h"

#include "imgui_internal.h"
#include "ImGuizmo.h"


ImGuizmoManager& ImGuizmoManager::Get()
{
    static ImGuizmoManager Instance;
    return Instance;
}

void ImGuizmoManager::BeginFrame(HWND hWnd)
{
    ImGuizmo::BeginFrame();
    
    RECT rect;
    ::GetClientRect(hWnd, &rect);
    float width  = static_cast<float>(rect.right  - rect.left);
    float height = static_cast<float>(rect.bottom - rect.top);

    ImGuiIO& io = ImGui::GetIO();
    
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetCurrentWindow()->DrawList);
    ImGuizmo::SetRect(0, 0, width, height);
}