#pragma once
#include <stdafx.h>
#include <D3DHelpler/TextureManager.h>
#include <Framework/ImguiManager.h>
#include "MaterialManager.h"
#include "Tools/CPUTimer.h"
#include "Tools/D3D12GpuTimer.h"

class GResource {
public:
    inline static std::unique_ptr<TextureManager> TextureManager = nullptr;
    inline static std::unique_ptr<ImguiManager> GUIManager = nullptr;
    inline static std::unique_ptr<MaterialManager> MaterialManager = nullptr;
    inline static std::unique_ptr<CPUTimer> CPUTimerManager = nullptr;
    inline static std::unique_ptr<D3D12GpuTimer> GPUTimer = nullptr;
    inline static uint Width = 0;
    inline static uint Height = 0;
};
