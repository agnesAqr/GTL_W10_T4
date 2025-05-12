#include "AnimationPreviewUI.h"
#include "EditorPanel.h"
#include "PropertyEditor/AnimTimelinePanel.h"
#include "PropertyEditor/PreviewControlEditorPanel.h"
#include "PropertyEditor/PrimitiveDrawEditor.h"
#include "PropertyEditor/PropertyEditorPanel.h"

void FAnimationPreviewUI::Initialize(SLevelEditor* LevelEditor, float Width, float Height)
{
    auto ControlPanel = std::make_shared<PreviewControlEditorPanel>();
    ControlPanel->Initialize(LevelEditor, Width, Height);
    Panels["PreviewControlPanel"] = ControlPanel;

    auto AnimTimelinePanel = std::make_shared<::AnimTimelinePanel>();
    AnimTimelinePanel->Initialize(LevelEditor, Width, Height);
    Panels["AnimTimelinePanel"] = AnimTimelinePanel;
}

void FAnimationPreviewUI::Render() const
{
    for (const auto& Panel : Panels)
    {
        Panel.Value->Render();
    }
}

void FAnimationPreviewUI::AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel)
{
    Panels[PanelId] = EditorPanel;
}

void FAnimationPreviewUI::OnResize(HWND hWnd) const
{
    for (auto& Panel : Panels)
    {
        Panel.Value->OnResize(hWnd);
    }
}

void FAnimationPreviewUI::SetWorld(UWorld* InWorld)
{
    World = InWorld;
    for (auto& [_, Panel] : Panels)
    {
        Panel->SetWorld(World);
    } 
}

std::shared_ptr<UEditorPanel> FAnimationPreviewUI::GetEditorPanel(const FString& PanelId)
{
    return Panels[PanelId];
}
