#pragma once
#include <string>

#include "Stat.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Delegates/DelegateCombination.h"
#include "PropertyEditor/IWindowToggleable.h"

#define UE_LOG Console::GetInstance().AddLog

DECLARE_DELEGATE(FOnCPUSkinning)
DECLARE_DELEGATE(FOnGPUSkinning)

enum class LogLevel
{
    Display,
    Warning,
    Error
};

class Console : public IWindowToggleable
{
private:
    Console();
    ~Console();
public:
    static Console& GetInstance(); // 참조 반환으로 변경

    void Clear();
    void AddLog(LogLevel level, const char* fmt, ...);
    void Draw();
    void ExecuteCommand(const std::string& command);
    void OnResize(HWND hWnd);
    void Toggle() override
    { 
        if (bWasOpen)
        {
            bWasOpen = false;
        }
        else
        {
            bWasOpen = true;
        }
    } // Toggle() 구현
    
    void SetShadowFilterMode(const std::string& command);

    FOnCPUSkinning OnCPUSkinning;
    FOnGPUSkinning OnGPUSkinning;
public:
    struct LogEntry
    {
        LogLevel level;
        FString message;
    };

    TArray<LogEntry> items;
    TArray<FString> history;
    int32 historyPos = -1;
    char inputBuf[256] = "";
    bool scrollToBottom = false;
    
    // 추가된 멤버 변수들
    bool showLogTemp = true;   // LogTemp 체크박스
    bool showWarning = true;   // Warning 체크박스
    bool showError = true;     // Error 체크박스

    bool bWasOpen = true;
    bool showFPS = false;
    bool showMemory = false;
    // 복사 방지
    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;
    Console(Console&&) = delete;
    Console& operator=(Console&&) = delete;

    FStatOverlay overlay;
private:
    bool bExpand = true;
    uint32 width;
    uint32 height;
};
