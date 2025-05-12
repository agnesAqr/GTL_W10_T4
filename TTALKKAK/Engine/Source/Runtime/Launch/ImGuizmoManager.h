#pragma once
#include "D3D11RHI/GraphicDevice.h"

class ImGuizmoManager
{
public:

    static ImGuizmoManager& Get();

    void BeginFrame(HWND hWnd);
};
