#include "Stat.h"

#define _TCHAR_DEFINED
#include <d3d11.h>

#include "LaunchEngineLoop.h"
#include "Actors/ADodge.h"
#include "Animation/AnimationSettings.h"
#include "ImGUI/imgui.h"
#include "Light/ShadowResource.h"
#include "Math/MathUtility.h"
#include "Renderer/Renderer.h"

GPUStat::GPUStat(const FString& InName)
    : Name(InName)
{

}

void GPUStat::Init(ID3D11Device* InDevice)
{
    assert(InDevice);
    D3D11_QUERY_DESC d{ D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
    HRESULT hr = InDevice->CreateQuery(&d, &DisjointQuery);
    assert(SUCCEEDED(hr));
    
    d.Query = D3D11_QUERY_TIMESTAMP;
    hr = InDevice->CreateQuery(&d, &StartQuery);
    assert(SUCCEEDED(hr));
    
    hr = InDevice->CreateQuery(&d, &EndQuery);
    assert(SUCCEEDED(hr));
    
    bIsInitialize = true;  // ← 잊지 말고 세팅
}

void GPUStat::Begin(ID3D11DeviceContext* InDeviceContext)
{
    if (!bIsInitialize) return;
    
    InDeviceContext->Begin(DisjointQuery);
    InDeviceContext->End(StartQuery);
}

void GPUStat::End(ID3D11DeviceContext* InDeviceContext)
{
    if (!bIsInitialize) return;
    
    InDeviceContext->End(EndQuery);
    InDeviceContext->End(DisjointQuery);

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT info;
    while (InDeviceContext->GetData(DisjointQuery, &info, sizeof(info), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_FALSE);
    if (info.Disjoint) return;

    uint64 t0 = 0, t1 = 0;
    InDeviceContext->GetData(StartQuery, &t0, sizeof(t0), 0);
    InDeviceContext->GetData(EndQuery,   &t1, sizeof(t1), 0);
    const double ms = static_cast<double>(t1 - t0) * 1000.0 / static_cast<double>(info.Frequency);
    TotalMs += ms;
    MinMs    = FMath::Min(MinMs, ms);
    MaxMs    = FMath::Max(MaxMs, ms);
    ++Count;
}

double GPUStat::Average() const
{
    return Count ? (TotalMs / Count) : 0.0;
}

void GPUStat::Reset()
{
    Count   = 0;
    TotalMs = 0;
    MinMs   = DBL_MAX;
    MaxMs   = 0;
}

void FStatOverlay::ToggleStat(const std::string& command)
{
    if (command == "stat fps")
    {
        showFPS = true; showRender = true; isOpen = true;
    }
    else if (command == "stat memory")
    {
        showMemory = true; showRender = true; isOpen = true;
    }
    else if (command == "stat shadow")
    {
        showShadow = true; showRender = true; isOpen = true;
    }
    else if (command == "stat skinning")
    {
        CPUSkinStat.Reset();
        GPUSkinStat.Reset();
        showSkin = true; showRender = true; isOpen = true;
    }
    else if (command == "stat none")
    {
        showFPS = false;
        showMemory = false;
        showRender = false;
        isOpen = false;
    }
}

void FStatOverlay::Render()
{
    if (!showRender || !isOpen)
        return;

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImVec2 windowSize(displaySize.x * 0.5f, displaySize.y * 0.5f);
    ImVec2 windowPos((displaySize.x - windowSize.x) * 0.5f, (displaySize.y - windowSize.y) * 0.5f);

    //ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    //ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

    ImGui::Begin("Stat Overlay", &isOpen);

    if (showFPS)
    {
        static float lastTime = ImGui::GetTime();
        static int frameCount = 0;
        static float fps = 0.0f;

        frameCount++;
        float currentTime = ImGui::GetTime();
        float deltaTime = currentTime - lastTime;

        if (deltaTime >= 1.0f)
        {
            fps = frameCount / deltaTime;
            frameCount = 0;
            lastTime = currentTime;
        }
        ImGui::Text("FPS: %.2f", fps);
        ImGui::Separator();
    }

    if (showMemory)
    {
        ImGui::Text("Allocated Object Count: %llu", FPlatformMemory::GetAllocationCount<EAT_Object>());
        ImGui::Text("Allocated Object Memory: %llu B", FPlatformMemory::GetAllocationBytes<EAT_Object>());
        ImGui::Text("Allocated Container Count: %llu", FPlatformMemory::GetAllocationCount<EAT_Container>());
        ImGui::Text("Allocated Container Memory: %llu B", FPlatformMemory::GetAllocationBytes<EAT_Container>());

        ImGui::Separator();
    }

    if (showShadow)
    {
        FShadowMemoryUsageInfo Info = FShadowResourceFactory::GetShadowMemoryUsageInfo();
        ImGui::Text("Shadow Memory Usage Info:");
        size_t pointlightAtlasMemory = GEngineLoop.Renderer.GetAtlasMemoryUsage(ELightType::PointLight);
        size_t spotlightAtlasMemory = GEngineLoop.Renderer.GetAtlasMemoryUsage(ELightType::SpotLight);
        ImGui::Text("PointLight Atlas Memory Usage : %.2f MB", static_cast<float>(pointlightAtlasMemory) / (1024.f * 1024.f));
        ImGui::Text("SpotLight Atlas Memory Usage : %.2f MB", static_cast<float>(spotlightAtlasMemory) / (1024.f * 1024.f));

        float total = static_cast<float>(Info.TotalMemoryUsage + pointlightAtlasMemory + spotlightAtlasMemory) / (1024.f * 1024.f);
        ImGui::Text("Total Memory: %.2f MB", total);
        for (const auto& pair : Info.MemoryUsageByLightType)
        {
            switch (pair.Key)
            {
            case ELightType::DirectionalLight:
            {
                ImGui::Text("%d Directional Light", Info.LightCountByLightType[pair.Key] / 4); // cascade때문에 4개 
                const float mb = static_cast<float>(pair.Value) / (1024.f * 1024.f);
                ImGui::Text("Memory: %.2f MB", mb);
            }
                break;
            case ELightType::PointLight:
                ImGui::Text("%d Point Light", Info.LightCountByLightType[pair.Key]);
                break;
            case ELightType::SpotLight:
                ImGui::Text("%d Spot Light", Info.LightCountByLightType[pair.Key]);
                break;
            default:
                break;
            }
        }
    }

    if (showSkin)
    {
        ImGui::Separator();
        if (GCurrentSkinningMode == ESkinningMode::CPU)
        {
            ImGui::Text(
                "CPU Skinning: avg=%.2f ms  (min=%.2f ms, max=%.2f ms)",
                CPUSkinStat.Average(), CPUSkinStat.MinMs, CPUSkinStat.MaxMs
            );
        }
        else if (GCurrentSkinningMode == ESkinningMode::GPU)
        {
            if (GPUSkinStat.bIsInitialize)
            {
                ImGui::Text(
                    "GPU Skinning: avg=%.2f ms  (min=%.2f ms, max=%.2f ms)",
                    GPUSkinStat.Average(), GPUSkinStat.MinMs, GPUSkinStat.MaxMs
                );
            }
        }
    }

    ImGui::PopStyleColor();
    ImGui::End();

    if (!isOpen)
    {
        showRender = false;
    }
}


float FStatOverlay::CalculateFPS()
{
    static int frameCount = 0;
    static float elapsedTime = 0.0f;
    static float lastTime = 0.0f;

    const float currentTime = GetTickCount64() / 1000.0f;
    elapsedTime += (currentTime - lastTime);
    lastTime = currentTime;
    frameCount++;

    if (elapsedTime > 1.0f) {
        const float fps = frameCount / elapsedTime;
        frameCount = 0;
        elapsedTime = 0.0f;
        return fps;
    }
    return 0.0f;
}

void FStatOverlay::DrawTextOverlay(const std::string& InText, const int InX, const int InY)
{
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(InX), static_cast<float>(InY)));
    ImGui::Begin("##stat", nullptr,
                 ImGuiWindowFlags_NoBackground|
                 ImGuiWindowFlags_NoTitleBar|
                 ImGuiWindowFlags_NoInputs|
                 ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("%s", InText);
    ImGui::End();
}