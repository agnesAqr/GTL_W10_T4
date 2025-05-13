#include "AnimTimelinePanel.h"

#include "EditorEngine.h"
#include "imgui.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "Engine/FEditorStateManager.h"
#include "im-neo-sequencer/imgui_neo_sequencer.h"

void AnimTimelinePanel::Initialize(SLevelEditor* levelEditor, float Width, float Height)
{
    activeLevelEditor = levelEditor;
    this->Width = Width;
    this->Height = Height;
}

void AnimTimelinePanel::Render()
{
    ImGuiIO& io = ImGui::GetIO();
    IconFont = io.Fonts->Fonts[FEATHER_FONT];
    
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
    for (auto Actor : World->GetActors())
    {
        if (ASkeletalMeshActor* SkeletalMeshActor = Cast<ASkeletalMeshActor>(Actor))
        {
            SkeletalMeshComponent = SkeletalMeshActor->GetComponentByClass<USkeletalMeshComponent>();
            // TODO : CurrAnimSeq 가져오는 방식 바꿔야 함
            UAnimSingleNodeInstance* singleNodeInstance = Cast<UAnimSingleNodeInstance>(SkeletalMeshComponent->GetAnimInstance());
            CurrAnimSeq = singleNodeInstance->GetAnimation();
        }
    }

    if (!CurrAnimSeq) return;
    
    const float fps            = CurrAnimSeq->GetSamplingFrameRate().AsDecimal();
    const int32_t totalFrames  = CurrAnimSeq->GetNumberOfFrames();
    static int32_t currentFrame = 0;
    static int32_t startFrame   = std::max(0, std::min(startFrame, totalFrames - 1));
    static int32_t endFrame     = std::max(1, std::min(endFrame, totalFrames));
    static int     playbackDir  = 0;
    static double  lastTime     = ImGui::GetTime();

    if (endFrame == 0)
        endFrame = totalFrames;

    double now = ImGui::GetTime();
    if (playbackDir != 0)
    {
        double delta = now - lastTime;
        int32_t adv = int32_t(delta * fps) * playbackDir;
        if (adv != 0)
        {
            currentFrame = (currentFrame + adv) % totalFrames;
            if (currentFrame < 0) currentFrame += totalFrames;
            lastTime = now;
        }
    }
    lastTime = (playbackDir != 0) ? lastTime : now;

    ImGui::Begin("Animation Sequencer");
    
    // ◀ Reverse 버튼
    if (ImGui::Button("Reverse"))
        playbackDir = -1;
    
    ImGui::PushFont(IconFont);
    ImGui::SameLine();
    // ■ Pause 버튼
    if (ImGui::Button("\ue99c"))
        playbackDir = 0;
    ImGui::SameLine();
    // ▶ Play 버튼
    if (ImGui::Button("\ue9a8"))
        playbackDir = 1;
    ImGui::PopFont();
    
    ImGui::SameLine();
    if (ImGui::SliderInt("Frame", &currentFrame, 0, totalFrames - 1))
    {
        lastTime = now;
    }

    std::vector<ImGui::FrameIndexType> notifyFrames;
    for (auto& ev : CurrAnimSeq->GetNotifies())
    {
        notifyFrames.push_back(FMath::RoundToInt(ev.TriggerTime * fps));
    }
    
    const int seqFlags =
          ImGuiNeoSequencerFlags_AllowLengthChanging
        | ImGuiNeoSequencerFlags_EnableSelection
        | ImGuiNeoSequencerFlags_AlwaysShowHeader
        | ImGuiNeoSequencerFlags_Selection_EnableDragging
        | ImGuiNeoSequencerFlags_Selection_EnableDeletion;

    // Sequencer 그리기
    if (ImGui::BeginNeoSequencer("Sequencer", &currentFrame, &startFrame, &endFrame, ImVec2(0,0), seqFlags))
    {
        // 1. 빈 공간 우클릭 시 Notify 추가 메뉴
        if (ImGui::BeginPopupContextWindow("add_notify", ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Add Notify at Current"))
            {
                FAnimNotifyEvent newEv;
                newEv.TriggerTime = float(currentFrame) / fps;
                CurrAnimSeq->GetNotifies().Add(newEv);
            }
            ImGui::EndPopup();
        }

        // 2. Notify 트랙
        bool showNotifies = true;
        if (ImGui::BeginNeoTimeline("Notifies", notifyFrames, &showNotifies,
                                    ImGuiNeoTimelineFlags_AllowFrameChanging))
        {
            ImGui::EndNeoTimeLine();
        }

        // 3. 선택된 키프레임 삭제 처리
        if (ImGui::NeoCanDeleteSelection())
        {
            uint32_t count = ImGui::GetNeoKeyframeSelectionSize();
            std::vector<ImGui::FrameIndexType> toRemove(count);
            ImGui::GetNeoKeyframeSelection(toRemove.data());

            // 역순으로 지워야 인덱스 꼬임 방지
            std::sort(toRemove.begin(), toRemove.end(), std::greater<int32_t>());
            for (auto idx : toRemove)
                CurrAnimSeq->GetNotifies().RemoveAt(idx);

            ImGui::NeoClearSelection();
        }
        ImGui::EndNeoSequencer();
    }
    ImGui::End();

    if (SkeletalMeshComponent)
    {
        float time = CurrAnimSeq->GetTimeAtFrame(currentFrame);
        SkeletalMeshComponent->SetPosition(time, false);
    }
}