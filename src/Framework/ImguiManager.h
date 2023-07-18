
#pragma once
#include <stdafx.h>
#include "D3DHelpler/DescroptorHeap.h"

class ImguiManager {
    struct GUIState {
        float RenderTime;
        float RenderGPUTime;
        float DrawCallTime;
        float UpdateSceneTime;
        float UpdatePassTime;
        float GBufferTime;
        float ComputeShaderTime;
        float AvgFPS;
        bool EnableSphere = false;
        bool EnableModel = true;
        uint FPSCount;
        char CameraSpeed[10];
    };

public:
    ImguiManager();
    ~ImguiManager();
    ImguiManager(const ImguiManager &) = default;
    ImguiManager &operator=(const ImguiManager &) = default;
    ImguiManager(ImguiManager &&) = delete;
    ImguiManager &operator=(ImguiManager &&) = delete;

    void Init(ID3D12Device *device);
    void DrawUI(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *target);

    GUIState State;

private:
    std::unique_ptr<DescriptorHeap>
        mUiSrvHeap;
};