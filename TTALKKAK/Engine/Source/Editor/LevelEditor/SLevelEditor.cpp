#include "SLevelEditor.h"

#include "ImGuiManager.h"

#include "Slate/Widgets/Layout/SSplitter.h"
#include "Viewport.h"
#include "UnrealEd/EditorViewportClient.h"
#include "LaunchEngineLoop.h"
#include "WindowsCursor.h"
#include "Engine/World.h"

extern UEngine* GEngine;

SLevelEditor::SLevelEditor()
{
}

void SLevelEditor::Initialize(UWorld* World, HWND OwnerWindow)
{
    ActiveViewportWindow = OwnerWindow;
    ActiveViewportClientIndex = 0;
    for (size_t i = 0; i < 4; i++)
    {
        std::shared_ptr<FEditorViewportClient> EditorViewportClient = AddViewportClient<FEditorViewportClient>(OwnerWindow, World);
        EViewScreenLocation Location = static_cast<EViewScreenLocation>(i);
        EditorViewportClient->GetViewport()->ViewScreenLocation = Location;
    }
    
    std::shared_ptr<SSplitterV> VSplitter = std::make_shared<SSplitterV>();
    VSplitter->Initialize(FRect(0, 0, WindowViewportDataMap[OwnerWindow].EditorWidth, WindowViewportDataMap[OwnerWindow].EditorHeight));
    WindowViewportDataMap[OwnerWindow].VSplitter = VSplitter;

    std::shared_ptr<SSplitterH> HSplitter = std::make_shared<SSplitterH>();
    HSplitter->Initialize(FRect(0, 0, WindowViewportDataMap[OwnerWindow].EditorWidth, WindowViewportDataMap[OwnerWindow].EditorHeight));
    WindowViewportDataMap[OwnerWindow].HSplitter = HSplitter;

    // Check Resize
    
    LoadConfig();

    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();
    
    Handler->OnMouseDownDelegate.AddLambda([this](const FPointerEvent& InMouseEvent, HWND AppWnd)
    {
        if (ImGuiManager::Get().GetWantCaptureMouse(AppWnd)) return;

        if (!WindowViewportDataMap.Contains(AppWnd))
        {
            return;
        }
        FWindowViewportClientData& WindowViewportData = WindowViewportDataMap[AppWnd];

        if (WindowViewportData.ViewportClients.Num() == 0)
        {
            return;
        }

        UWorld* World = WindowViewportData.ViewportClients[0]->GetWorld();
        if (World->WorldType != EWorldType::Editor && World->WorldType != EWorldType::EditorPreview)
        {
            return;
        }
        
        switch (InMouseEvent.GetEffectingButton())  // NOLINT(clang-diagnostic-switch-enum)
        {
        case EKeys::RightMouseButton:
        {
            if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
            {
                FWindowsCursor::SetShowMouseCursor(false);
                MousePinPosition = InMouseEvent.GetScreenSpacePosition();
            }
            break;
        }
        default:
            break;
        }

        FVector2D ClientPos = FWindowsCursor::GetClientPosition(AppWnd);
        SelectViewport(AppWnd, ClientPos);
        
        // 마우스 이벤트가 일어난 위치의 뷰포트를 선택
        if (WindowViewportData.VSplitter)
        {
            WindowViewportData.VSplitter->OnPressed(FPoint(ClientPos.X, ClientPos.Y));
        }
        if (WindowViewportData.HSplitter)
        {
            WindowViewportData.HSplitter->OnPressed(FPoint(ClientPos.X, ClientPos.Y));
        }
    });
    
    Handler->OnMouseMoveDelegate.AddLambda([this](const FPointerEvent& InMouseEvent, HWND AppWnd)
    {
        if (ImGuiManager::Get().GetWantCaptureMouse(AppWnd))
        {
            return;
        }

        if (!WindowViewportDataMap.Contains(AppWnd))
        {
            return;
        }
        
        FWindowViewportClientData& WindowViewportData = WindowViewportDataMap[AppWnd];
    
        // Splitter 움직임 로직
        if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
        {
            const auto& [DeltaX, DeltaY] = InMouseEvent.GetCursorDelta();
            
            bool bSplitterDragging = false;
            if (WindowViewportData.VSplitter && WindowViewportData.VSplitter->IsSplitterPressed())
            {
                WindowViewportData.VSplitter->OnDrag(FPoint(DeltaX, DeltaY));
                bSplitterDragging = true;
            }
            if (WindowViewportData.HSplitter && WindowViewportData.HSplitter->IsSplitterPressed())
            {
                WindowViewportData.HSplitter->OnDrag(FPoint(DeltaX, DeltaY));
                bSplitterDragging = true;
            }
    
            if (bSplitterDragging)
            {
                ResizeViewports(AppWnd);
            }
        }
    
        // 멀티 뷰포트일 때, 커서 변경 로직
        if (WindowViewportData.bMultiViewportMode && !InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
        {
            // TODO: 나중에 커서가 Viewport 위에 있을때만 ECursorType::Crosshair로 바꾸게끔 하기
            // ECursorType CursorType = ECursorType::Crosshair;
            ECursorType CursorType = ECursorType::Arrow;
            
            FVector2D ClientPos = FWindowsCursor::GetClientPosition(AppWnd);
            const bool bIsVerticalHovered = WindowViewportData.VSplitter ? WindowViewportData.VSplitter->IsHover(FPoint{ClientPos.X, ClientPos.Y}) : false;
            const bool bIsHorizontalHovered = WindowViewportData.HSplitter ? WindowViewportData.HSplitter->IsHover(FPoint{ClientPos.X, ClientPos.Y}) : false;
    
            if (bIsHorizontalHovered && bIsVerticalHovered)
            {
                CursorType = ECursorType::ResizeAll;
            }
            else if (bIsHorizontalHovered)
            {
                CursorType = ECursorType::ResizeLeftRight;
            }
            else if (bIsVerticalHovered)
            {
                CursorType = ECursorType::ResizeUpDown;
            }
            FWindowsCursor::SetMouseCursor(CursorType);
        }

        std::shared_ptr<FEditorViewportClient> ActiveViewportClient = WindowViewportData.GetActiveViewportClient();

        if (WindowViewportData.ViewportClients.Num() == 0)
        {
            return;
        }

        UWorld* World = WindowViewportData.ViewportClients[0]->GetWorld();
        if (World->WorldType != EWorldType::Editor && World->WorldType != EWorldType::EditorPreview)
        {
            return;
        }
        
        if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
        {
            // Mouse Move 이벤트 일때만 실행
            if (InMouseEvent.GetInputEvent() == IE_Axis && InMouseEvent.GetEffectingButton() == EKeys::Invalid)
            {
                // 에디터 카메라 이동 로직
                if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
                {
                    ActiveViewportClient->MouseMove(InMouseEvent);
                }
            }
        }
    });
    
    Handler->OnMouseUpDelegate.AddLambda([this](const FPointerEvent& InMouseEvent, HWND AppWnd)
    {
        if (!WindowViewportDataMap.Contains(AppWnd))
        {
            return;
        }

        FWindowViewportClientData& WindowViewportData = WindowViewportDataMap[AppWnd];
        
        switch (InMouseEvent.GetEffectingButton())  // NOLINT(clang-diagnostic-switch-enum)
        {
        case EKeys::RightMouseButton:
            {
                FWindowsCursor::SetShowMouseCursor(true);
                FWindowsCursor::SetPosition(
                    static_cast<int32>(MousePinPosition.X),
                    static_cast<int32>(MousePinPosition.Y)
                );
                break;
            }
        case EKeys::LeftMouseButton:
            {
                if (WindowViewportData.VSplitter)
                {
                    WindowViewportData.VSplitter->OnReleased();
                }
                if (WindowViewportData.HSplitter)
                {
                    WindowViewportData.HSplitter->OnReleased();
                }
                break;
            }
        default:
            break;
        }
    });
    
    Handler->OnMouseWheelDelegate.AddLambda([this](const FPointerEvent& InMouseEvent, HWND AppWnd)
    {
        if (!WindowViewportDataMap.Contains(AppWnd))
        {
            return;
        }
        
        FWindowViewportClientData& WindowViewportClientData = WindowViewportDataMap[AppWnd];
        
        if (ImGuiManager::Get().GetWantCaptureMouse(AppWnd))
        {
            return;
        }

        std::shared_ptr<FEditorViewportClient> ActiveViewportClient = WindowViewportClientData.GetActiveViewportClient();
        
        // 뷰포트에서 앞뒤 방향으로 화면 이동
        if (ActiveViewportClient->IsPerspective())
        {
            if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
            {
                // 카메라 속도 조절
                if (ActiveViewportClient->IsPerspective())
                {
                    const float CurrentSpeed = ActiveViewportClient->GetCameraSpeedScalar();
                    const float Adjustment = FMath::Sign(InMouseEvent.GetWheelDelta()) * FMath::Loge(CurrentSpeed + 1.0f) * 0.5f;
            
                    ActiveViewportClient->SetCameraSpeed(CurrentSpeed + Adjustment);
                }
            }
            else
            {
                const FVector CameraLoc = ActiveViewportClient->ViewTransformPerspective.GetLocation();
                const FVector CameraForward = ActiveViewportClient->ViewTransformPerspective.GetForwardVector();
                ActiveViewportClient->ViewTransformPerspective.SetLocation(CameraLoc + CameraForward * InMouseEvent.GetWheelDelta() * 50.0f);
            }
        }
        else
        {
            // TODO Ortho Static 빼야됨.
            FEditorViewportClient::SetOrthoSize(FEditorViewportClient::GetOrthoSize() + (-InMouseEvent.GetWheelDelta()));
        }
    });

    Handler->OnKeyDownDelegate.AddLambda([this](const FKeyEvent& InKeyEvent, HWND AppWnd)
    {
        if (!WindowViewportDataMap.Contains(AppWnd))
        {
            return;
        }
        
        FWindowViewportClientData& WindowViewportClientData = WindowViewportDataMap[AppWnd];
        WindowViewportClientData.GetActiveViewportClient()->InputKey(AppWnd, InKeyEvent);

        if (GetKeyState(VK_RBUTTON) & 0x8000)
        {
            return;
        }
        
        if (InKeyEvent.GetInputEvent() != IE_Pressed)
        {
            return;
        }
        
        switch (InKeyEvent.GetCharacter())
        {
        case 'Q':
            {
                //GetWorld()->SetPickingObj(nullptr);
                break;
            }
        case 'W':
            {
                WindowViewportClientData.SetMode(CM_TRANSLATION);
                break;
            }
        case 'E':
            {
                WindowViewportClientData.SetMode(CM_ROTATION);
                break;
            }
        case 'R':
            {
                WindowViewportClientData.SetMode(CM_SCALE);
                break;
            }
        default:
            break;
        }
    });
    
    Handler->OnKeyUpDelegate.AddLambda([this](const FKeyEvent& InKeyEvent, HWND AppWnd)
    {
        if (!WindowViewportDataMap.Contains(AppWnd))
        {
            return;
        }
        
        FWindowViewportClientData WindowViewportClientData = WindowViewportDataMap[AppWnd];
        WindowViewportClientData.GetActiveViewportClient()->InputKey(AppWnd, InKeyEvent);
    });
}

void SLevelEditor::Tick(double DeltaTime)
{
    for (auto& [AppWnd, WindowViewportData] : WindowViewportDataMap)
    {
        for (std::shared_ptr<FEditorViewportClient>& ViewportClient : WindowViewportData.ViewportClients)
        {
            ViewportClient->Tick(DeltaTime);
        }
    }
}

void SLevelEditor::Release()
{
    WindowViewportDataMap.Empty();
}

template <typename T>
    requires std::derived_from<T, FViewportClient>
std::shared_ptr<T> SLevelEditor::AddViewportClient(HWND OwnerWindow, UWorld* World)
{
    if (!WindowViewportDataMap.Contains(OwnerWindow))
    {
        WindowViewportDataMap.Add(OwnerWindow, FWindowViewportClientData());
        WindowViewportDataMap[OwnerWindow].EditorWidth = GEngineLoop.GraphicDevice.SwapChains[OwnerWindow].ScreenWidth;
        WindowViewportDataMap[OwnerWindow].EditorHeight = GEngineLoop.GraphicDevice.SwapChains[OwnerWindow].ScreenHeight;
    }
    std::shared_ptr<T> ViewportClient = std::make_shared<T>();
    ViewportClient->Initialize(OwnerWindow, WindowViewportDataMap[OwnerWindow].ViewportClients.Num(), World);

    WindowViewportDataMap[OwnerWindow].ViewportClients.Add(ViewportClient);
    return ViewportClient;
}

void SLevelEditor::RemoveViewportClient(HWND OwnerWindow, const std::shared_ptr<FEditorViewportClient> ViewportClient)
{
    if (!WindowViewportDataMap.Contains(OwnerWindow))
    {
        return;
    }
    
    // ViewportClient 소멸자에서 해줌.
    // ViewportClient->Release();
    
    FWindowViewportClientData& WindowViewportData = WindowViewportDataMap[OwnerWindow];
    WindowViewportData.ViewportClients.Remove(ViewportClient);
    if (WindowViewportData.ViewportClients.Num() == 0)
    {
        WindowViewportDataMap.Remove(OwnerWindow);
        
        ActiveViewportWindow = nullptr;
        ActiveViewportClientIndex = 0;
    }
    else
    {
        for (uint32 i = 0; i < WindowViewportData.ViewportClients.Num(); i++)
        {
            WindowViewportDataMap[OwnerWindow].ViewportClients[i]->SetViewportIndex(i);
        }
        
        ActiveViewportWindow = WindowViewportDataMap.begin()->Key;
        ActiveViewportClientIndex = WindowViewportDataMap.begin()->Value.ActiveViewportIndex;
    }
}

void SLevelEditor::RemoveViewportClients(HWND HWnd)
{
    // ViewportClient 소멸자에서 해줌.
    // for (auto& ViewportClient : WindowViewportDataMap[HWnd].ViewportClients)
    // {
    //     ViewportClient->Release();
    // }
    WindowViewportDataMap.Remove(HWnd);
    if (WindowViewportDataMap.Num() > 0)
    {
        ActiveViewportWindow = WindowViewportDataMap.begin()->Key;
        ActiveViewportClientIndex = WindowViewportDataMap.begin()->Value.ActiveViewportIndex;
    }
    else
    {
        ActiveViewportWindow = nullptr;
        ActiveViewportClientIndex = 0;
    }
}

void SLevelEditor::SelectViewport(HWND AppWnd, FVector2D Point)
{
    if (!WindowViewportDataMap.Contains(AppWnd))
    {
        return;
    }

    FWindowViewportClientData& WindowViewportData = WindowViewportDataMap[AppWnd];
    std::shared_ptr<FEditorViewportClient> ActiveViewportClient = WindowViewportData.GetActiveViewportClient();
    
    for (uint32 Index = 0; Index < WindowViewportData.ViewportClients.Num(); Index++)
    {
        std::shared_ptr<FEditorViewportClient> ViewportClient = WindowViewportData.ViewportClients[Index];
        if (ViewportClient->GetViewport()->GetFSlateRect().Contains(Point))
        {
            FocusViewportClient(AppWnd, Index);
            break;
        }
    }
}

void SLevelEditor::ResizeWindow(HWND AppWnd, FVector2D ClientSize)
{
    if (!WindowViewportDataMap.Contains(AppWnd))
    {
        return;
    }

    FWindowViewportClientData& WindowViewportData = WindowViewportDataMap[AppWnd];

    WindowViewportData.EditorWidth = ClientSize.X;
    WindowViewportData.EditorHeight = ClientSize.Y;

    if (WindowViewportData.HSplitter)
    {
        WindowViewportData.HSplitter->Resize(WindowViewportData.EditorWidth, WindowViewportData.EditorHeight);
    }

    if (WindowViewportData.VSplitter)
    {
        WindowViewportData.VSplitter->Resize(WindowViewportData.EditorWidth, WindowViewportData.EditorHeight);
    }
    
    ResizeViewports(AppWnd);
}

void SLevelEditor::ResizeViewports(HWND AppWnd)
{
    if (!WindowViewportDataMap.Contains(AppWnd))
    {
        return;
    }

    FWindowViewportClientData& WindowViewportData = WindowViewportDataMap[AppWnd];
    
    if (WindowViewportData.bMultiViewportMode)
    {
        for (std::shared_ptr<FEditorViewportClient> EditorViewportClient : WindowViewportData.ViewportClients)
        {
            EditorViewportClient->ResizeViewport(
                WindowViewportData.VSplitter->SideLT->GetRect(), WindowViewportData.VSplitter->SideRB->GetRect(),
                WindowViewportData.HSplitter->SideLT->GetRect(), WindowViewportData.HSplitter->SideRB->GetRect());
        }
    }
    else
    {
        WindowViewportData.ViewportClients[WindowViewportData.ActiveViewportIndex]->ResizeViewport(
            FRect(0.0f, 0.0f, WindowViewportData.EditorWidth, WindowViewportData.EditorHeight));
    }
}

void SLevelEditor::SetEnableMultiViewport(HWND AppWnd, bool bIsEnable)
{
    if (WindowViewportDataMap[AppWnd].VSplitter != nullptr || WindowViewportDataMap[AppWnd].HSplitter != nullptr)
    {
        WindowViewportDataMap[AppWnd].bMultiViewportMode = bIsEnable;
        ResizeViewports(AppWnd);
    }
    else
    {
        WindowViewportDataMap[AppWnd].bMultiViewportMode = false;   
    }
}

bool SLevelEditor::IsMultiViewport(HWND AppWnd)
{
    return WindowViewportDataMap[AppWnd].bMultiViewportMode;
}

void SLevelEditor::LoadConfig()
{
    auto config = ReadIniFile(IniFilePath);

    FWindowViewportClientData& WindowViewportData = WindowViewportDataMap[GEngineLoop.GetDefaultWindow()];
    std::shared_ptr<FEditorViewportClient> ActiveViewportClient = WindowViewportData.GetActiveViewportClient();
    
    ActiveViewportClient->Pivot.X = GetValueFromConfig(config, "OrthoPivotX", 0.0f);
    ActiveViewportClient->Pivot.Y = GetValueFromConfig(config, "OrthoPivotY", 0.0f);
    ActiveViewportClient->Pivot.Z = GetValueFromConfig(config, "OrthoPivotZ", 0.0f);
    ActiveViewportClient->OrthoSize = GetValueFromConfig(config, "OrthoZoomSize", 10.0f);
    
    WindowViewportData.ActiveViewportIndex = GetValueFromConfig(config, "ActiveViewportIndex", 0);
    WindowViewportData.bMultiViewportMode = GetValueFromConfig(config, "bMutiView", false);
    for (size_t i = 0; i < 4; i++)
    {
        WindowViewportData.ViewportClients[i]->LoadConfig(config);
    }
    
    if (WindowViewportData.HSplitter)
    {
        WindowViewportData.HSplitter->LoadConfig(config);
    }
    if (WindowViewportData.VSplitter)
    {
        WindowViewportData.VSplitter->LoadConfig(config);
    }

    for (auto& [AppWnd, WindowViewportData] : WindowViewportDataMap)
    {
        ResizeViewports(AppWnd);
    }
}

void SLevelEditor::SaveConfig()
{    
    TMap<FString, FString> config;

    FWindowViewportClientData& WindowViewportData = WindowViewportDataMap[GEngineLoop.GetDefaultWindow()];
    
    if (WindowViewportData.HSplitter)
        WindowViewportData.HSplitter->SaveConfig(config);
    if (WindowViewportData.VSplitter)
        WindowViewportData.VSplitter->SaveConfig(config);
    for (size_t i = 0; i < 4; i++)
    {
        WindowViewportData.ViewportClients[i]->SaveConfig(config);
    }
    std::shared_ptr<FEditorViewportClient> ActiveViewportClient = WindowViewportData.GetActiveViewportClient();
    ActiveViewportClient->SaveConfig(config);
    config["bMutiView"] = std::to_string(WindowViewportData.bMultiViewportMode);
    config["ActiveViewportIndex"] = std::to_string(ActiveViewportClient->GetViewportIndex());
    config["OrthoPivotX"] = std::to_string(ActiveViewportClient->Pivot.X);
    config["OrthoPivotY"] = std::to_string(ActiveViewportClient->Pivot.Y);
    config["OrthoPivotZ"] = std::to_string(ActiveViewportClient->Pivot.Z);
    config["OrthoZoomSize"] = std::to_string(ActiveViewportClient->OrthoSize);
    WriteIniFile(IniFilePath, config);
}

TMap<FString, FString> SLevelEditor::ReadIniFile(const FString& filePath)
{
    TMap<FString, FString> config;
    std::ifstream file(*filePath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[' || line[0] == ';') continue;
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            config[key] = value;
        }
    }
    return config;
}

void SLevelEditor::WriteIniFile(const FString& filePath, const TMap<FString, FString>& config)
{
    std::ofstream file(*filePath);
    for (const auto& pair : config) {
        file << *pair.Key << "=" << *pair.Value << "\n";
    }
}

