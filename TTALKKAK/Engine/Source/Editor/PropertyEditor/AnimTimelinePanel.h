#pragma once
#include "UnrealEd/EditorPanel.h"

struct ImFont;
class UAnimSequence;
class SLevelEditor;
class USkeletalMeshComponent;

class AnimTimelinePanel : public UEditorPanel
{
public:
    void Initialize(SLevelEditor* levelEditor, float Width, float Height);
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:
    SLevelEditor* activeLevelEditor;
    float Width = 300, Height = 100;
    USkeletalMeshComponent* SkeletalMeshComponent;
    UAnimSequence* CurrAnimSeq = nullptr;
    ImFont* IconFont;

    void DrawAnimationTimeline();
    void AddNotifyAtFrame(ANSICHAR* name, int32_t frame, float fps);
};