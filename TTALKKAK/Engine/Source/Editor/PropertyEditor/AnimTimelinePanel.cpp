#include "AnimTimelinePanel.h"

#include <Engine/Assets/Animation/AnimationAsset.h>

#include "EditorEngine.h"
#include "imgui.h"
#include "ImSequencer.h"
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
    float PanelWidth = (Width) * 1.0f;
    float PanelHeight = (Height) * 0.3f;

    float PanelPosX = (Width) * 0.8f + 5.0f;
    float PanelPosY = (Height) * 0.3f + 15.0f;

    ImVec2 MinSize(140, 370);
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
    // Todo : 선택한 SkeletalMeshComponent가 가진 UAnimInstance로 UAnimSequence 가져오기
    auto actors = GEngine->GetWorld()->GetActors();
    UAnimSequence* AnimSeq = nullptr;
    for (auto Actor : GEngine->GetWorld()->GetActors())
    {
        if (ASkeletalMeshActor* SkeletalMeshActor = Cast<ASkeletalMeshActor>(Actor))
            AnimSeq = SkeletalMeshActor->GetComponentByClass<USkeletalMeshComponent>()->GetAnimSequence();
    }
    
    if (!AnimSeq) return;

    static int   selectedEntry = -1;
    static int   firstFrame    = 0;
    static bool  expanded      = true;
    static int   currentFrame  = 0;

    const int FrameMin = 0;
    const int FrameMax = AnimSeq->GetNumberOfFrames();
    firstFrame = FrameMin;

    struct NotifySequence : public ImSequencer::SequenceInterface
    {
        UAnimSequence* Seq;
        NotifySequence(UAnimSequence* InSeq) : Seq(InSeq) {}

        int   GetFrameMin()   const override { return 0; }
        int   GetFrameMax()   const override { return Seq->GetNumberOfFrames(); }
        int   GetItemCount()  const override { return (int)Seq->GetNotifies().Num(); }
        int   GetItemTypeCount() const override { return 1; }
        const char* GetItemTypeName(int /*typeIndex*/) const override { return "Notify"; }
        const char* GetItemLabel(int index) const override
        {
            return *Seq->GetNotifies()[index].NotifyName.ToString();  // Notify Name
        }

        void Get(int index, int** start, int** end, int* type, unsigned int* color) override
        {
            static int sStart, sEnd;
            const auto& N = Seq->GetNotifies()[index];
            // 프레임 = TriggerTime × 프레임레이트
            float fps = Seq->GetSamplingFrameRate().AsDecimal();
            sStart = (int)roundf(N.TriggerTime * fps);
            sEnd   = sStart + (int)roundf(N.Duration * fps);
            if (start) *start = &sStart;
            if (end)   *end   = &sEnd;
            if (type) *type   = 0;
            if (color) *color = IM_COL32(200,100,100,255);
        }

        void Add(int /*type*/)         override {} 
        void Del(int /*index*/)       override {}
        void Duplicate(int /*index*/) override {}
        size_t GetCustomHeight(int /*index*/) override { return 0; }
        void DoubleClick(int /*index*/) override {}
    };

    NotifySequence seqInterface(AnimSeq);

    ImGui::Begin("Animation Sequencer");
    ImGui::PushItemWidth(-1);
    ImGui::PushItemWidth(-1); 
    Sequencer(&seqInterface,
              &currentFrame,
              &expanded,
              &selectedEntry,
              &firstFrame,
              ImSequencer::SEQUENCER_EDIT_STARTEND   // 시작/끝 프레임 드래그 가능
            | ImSequencer::SEQUENCER_ADD            // 아이템 추가
            | ImSequencer::SEQUENCER_DEL            // 아이템 삭제
            );
    ImGui::PopItemWidth();
    ImGui::End();
}


