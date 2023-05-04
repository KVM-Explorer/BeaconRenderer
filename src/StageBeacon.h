#pragma once
#include <stdafx.h>
#include "DataStruct.h"
#include "Resource/DeviceResource.h"
#include "Scene.h"
#include "Framework/RendererBase.h"
#include "Framework/GlobalResource.h"
#include "Pass/Pass.h"
#include "Resource/BackendResource.h"
#include "Resource/DisplayResource.h"
class StageBeacon : public RendererBase {
public:
    StageBeacon(uint width, uint height, std::wstring title);
    StageBeacon(const StageBeacon &) = delete;
    StageBeacon(StageBeacon &&) = default;
    StageBeacon &operator=(const StageBeacon &) = delete;
    StageBeacon &operator=(StageBeacon &&) = default;
    ~StageBeacon();

    void OnInit() override;

    void OnUpdate() override;
    void OnRender() override;
    void OnKeyDown(byte key) override;
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnDestory() override;

private:
    void LoadAssets();
    void CreateQuad();
    void PopulateCommandList();
    void CreateDeviceResource(HWND handle);
    void AddDisplayDevice(ID3D12Device *device);
    void AddBackendDevice(ID3D12Device *device);
    void CompileShaders();
    void CreateSignature2PSO();
    void CreateRtv();
    // =================== Property ===================

    CD3DX12_VIEWPORT mViewPort;
    CD3DX12_RECT mScissor;

    ComPtr<IDXGIFactory6> mFactory;

    std::unique_ptr<DisplayResource> mDisplayResource;
    std::vector<std::unique_ptr<BackendResource>> mBackendResource;
};