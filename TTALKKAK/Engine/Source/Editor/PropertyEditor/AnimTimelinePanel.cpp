#include "AnimTimelinePanel.h"

#include "EditorEngine.h"
#include "imgui.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "Engine/FEditorStateManager.h"
#include "im-neo-sequencer/imgui_neo_sequencer.h"
#include "Animation/AnimSingleNodeInstance.h"

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
            if (UAnimSingleNodeInstance* singleNodeInstance = Cast<UAnimSingleNodeInstance>(SkeletalMeshComponent->GetAnimInstance()))
            {
                CurrAnimSeq = singleNodeInstance->GetAnimation();
            }
        }
    }

    if (!CurrAnimSeq) return;
    
    const float fps            = CurrAnimSeq->GetSamplingFrameRate().AsDecimal();
    const int32_t totalFrames  = CurrAnimSeq->GetNumberOfFrames();
    
    static int32_t currentFrame = 0;
    static int32_t startFrame = 0;
    static int32_t endFrame   = 0;
    static int     playbackDir  = 0;
    static double  lastTime     = ImGui::GetTime();

    if (endFrame == 0)
        endFrame = totalFrames - 1;
    startFrame = std::clamp(startFrame, 0, totalFrames - 1);
    endFrame   = std::clamp(endFrame,   1, totalFrames);

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
    
    // ◀ Reverse, ■ Pause, ▶ Play
    if (ImGui::Button("Reverse")) playbackDir = -1;
    ImGui::PushFont(IconFont);
    ImGui::SameLine();
    if (ImGui::Button("\ue99c")) playbackDir = 0;
    ImGui::SameLine();
    if (ImGui::Button("\ue9a8")) playbackDir = 1;
    ImGui::PopFont();

    static bool     notifiesOpen    = true;
    static bool     openNotifyModal = false;
    static char     notifyNameBuffer[64] = "";
    static int32_t  inputFrame    = 0;
    static int32_t  pendingFrame    = 0;

    if (ImGui::BeginNeoSequencer("Sequencer", &currentFrame, &startFrame, &endFrame, {0,0},
                                 ImGuiNeoSequencerFlags_EnableSelection |
                                 ImGuiNeoSequencerFlags_Selection_EnableDragging |
                                 ImGuiNeoSequencerFlags_Selection_EnableDeletion))
    {
        if (ImGui::BeginNeoGroup("Notifies", &notifiesOpen))
        {
            TArray<ImGui::FrameIndexType> notifyFrames;
            for (auto& ev : CurrAnimSeq->GetNotifies())
                notifyFrames.Add(FMath::RoundToInt(ev.TriggerTime * fps));

            ImVec2 trackMin, trackMax;
            if (ImGui::BeginNeoTimelineEx("Track 1"))
            {
                for (auto f : notifyFrames)
                    ImGui::NeoKeyframe(&f);
                ImGui::EndNeoTimeLine();

                trackMin = ImGui::GetItemRectMin();
                trackMax = ImGui::GetItemRectMax();
            }

            ImGui::SetCursorScreenPos(trackMin);
            ImVec2 trackSize(trackMax.x - trackMin.x, trackMax.y - trackMin.y);
            ImGui::InvisibleButton("##track_ctx", trackSize);

            if (ImGui::BeginPopupContextItem("##track_ctx", ImGuiPopupFlags_MouseButtonRight))
            {
                if (ImGui::MenuItem("Add Notify"))
                {
                    ImVec2 mp = ImGui::GetMousePos();
                    float t   = (mp.x - trackMin.x) / trackSize.x;
                    t = FMath::Clamp(t, 0.f, 1.f);
                    pendingFrame = startFrame + int32(t * (endFrame - startFrame));

                    openNotifyModal = true;
                    inputFrame = pendingFrame;
                }
                ImGui::EndPopup();
            }
            
            if (ImGui::NeoCanDeleteSelection())
            {
                uint32_t n = ImGui::GetNeoKeyframeSelectionSize();
                std::vector<ImGui::FrameIndexType> toRemove(n);
                ImGui::GetNeoKeyframeSelection(toRemove.data());
                std::sort(toRemove.begin(), toRemove.end(), std::greater<>());
                for (auto idx : toRemove)
                    CurrAnimSeq->GetNotifies().RemoveAt(idx);
                ImGui::NeoClearSelection();
            }

            ImGui::EndNeoGroup();
        }
        ImGui::EndNeoSequencer();
    }

    ImGui::End();

    if (openNotifyModal)
    {
        openNotifyModal = false;
        ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
        ImGui::OpenPopup("Notify Popup");
    }
    if (ImGui::BeginPopupModal("Notify Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Name", notifyNameBuffer, IM_ARRAYSIZE(notifyNameBuffer));
        ImGui::InputInt("Frame", &inputFrame);
        inputFrame = FMath::Clamp(inputFrame, startFrame, endFrame);

        if (ImGui::Button("OK"))
        {
            AddNotifyAtFrame(notifyNameBuffer, inputFrame, fps);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (SkeletalMeshComponent)
    {
        float time = CurrAnimSeq->GetTimeAtFrame(currentFrame);
        SkeletalMeshComponent->SetPosition(time, false);
    }
}

void AnimTimelinePanel::AddNotifyAtFrame(ANSICHAR* name, int32_t frame, float fps)
{
    if (!CurrAnimSeq) return;

    FAnimNotifyEvent newEv;
    newEv.TriggerTime = float(frame) / fps;
    newEv.Duration = 0;
    newEv.NotifyName = FName(name);

    CurrAnimSeq->GetNotifies().Add(newEv);
}
