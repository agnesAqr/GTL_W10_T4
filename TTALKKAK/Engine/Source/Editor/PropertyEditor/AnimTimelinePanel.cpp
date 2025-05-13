#include "AnimTimelinePanel.h"

#include "EditorEngine.h"
#include "imgui.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "Engine/FEditorStateManager.h"

void AnimTimelinePanel::Initialize(SLevelEditor* levelEditor, float Width, float Height)
{
    activeLevelEditor = levelEditor;
    this->Width = Width;
    this->Height = Height;
}

void AnimTimelinePanel::Render()
{
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    if (EditorEngine == nullptr)
        return;

    /* Pre Setup */
    float PanelWidth = Width;
    float PanelHeight = (Height) * 0.3f;

    float PanelPosX = 0.0f;
    float PanelPosY = (Height) * 0.7f;

    ImVec2 MinSize(140, 140);
    ImVec2 MaxSize(FLT_MAX, 900);

    /* Min, Max Size */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

    /* Panel Position */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);

    /* Panel Size */
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    /* Draw Timeline */
    DrawAnimationTimeline();
}

void AnimTimelinePanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Width = clientRect.right - clientRect.left;
    Height = clientRect.bottom - clientRect.top;
}

void AnimTimelinePanel::DrawAnimationTimeline()
{
    
}



