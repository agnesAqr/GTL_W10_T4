// src/engine/profiler/Stat.h
#pragma once
#include "Container/String.h"

struct ID3D11DeviceContext;
struct ID3D11Query;
struct ID3D11Device;

struct Stat
{
    FString Name;
    uint64 Count   = 0;
    double TotalMs = 0;
    double MinMs   = DBL_MAX;
    double MaxMs   = 0;

    Stat(const std::string& n) : Name(n)
    {
        QueryPerformanceFrequency(&freq);
    }

    void Begin()
    {
        QueryPerformanceCounter(&start);
    }

    void End()
    {
        LARGE_INTEGER end;
        QueryPerformanceCounter(&end);
        const double ms = static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
        TotalMs += ms;
        MinMs    = std::min(MinMs, ms);
        MaxMs    = std::max(MaxMs, ms);
        ++Count;
    }

    double Average() const
    {
        return Count ? TotalMs / Count : 0.0;
    }
    
    void Reset()
    {
        Count   = 0;
        TotalMs = 0;
        MinMs   = DBL_MAX;
        MaxMs   = 0;
    }

private:
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
};

struct GPUStat
{
    FString Name;
    uint64 Count   = 0;
    double TotalMs = 0;
    double MinMs   = DBL_MAX;
    double MaxMs   = 0;

    bool bIsInitialize = false;

    ID3D11Query*   DisjointQuery = nullptr;
    ID3D11Query*   StartQuery    = nullptr;
    ID3D11Query*   EndQuery      = nullptr;

    GPUStat(const FString& InName);

    void Init(ID3D11Device* InDevice);

    void Begin(ID3D11DeviceContext* InDeviceContext);

    void End(ID3D11DeviceContext* InDeviceContext);

    double Average() const;

    void Reset();
};

class FStatOverlay 
{
public:
    bool showFPS = false;
    bool showMemory = false;
    bool showRender = false;
    bool showShadow = false;
    bool showSkin   = false;   // ← 스키닝 퍼포먼스 토글

    void ToggleStat(const std::string& command);
    
    void Render();

    Stat    CPUSkinStat{"CPU Skin"};
    GPUStat GPUSkinStat{"GPU Skin"};

    // 초기화 시 device 전달 필요
    void Init(ID3D11Device* device) { GPUSkinStat.Init(device); }
private:    
    bool isOpen = true;

    static float CalculateFPS();

    static void DrawTextOverlay(const std::string& InText, int InX, int InY);
};
