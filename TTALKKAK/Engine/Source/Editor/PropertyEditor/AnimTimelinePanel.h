#pragma once
#include "UnrealEd/EditorPanel.h"

class UAnimSequence;
class SLevelEditor;

class AnimTimelinePanel : public UEditorPanel
{
public:
    void Initialize(SLevelEditor* levelEditor, float Width, float Height);
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:
    SLevelEditor* activeLevelEditor;
    float Width = 300, Height = 100;
    UAnimSequence* CurrAnimSeq = nullptr;

    void DrawAnimationTimeline();
};