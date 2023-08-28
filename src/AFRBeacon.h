#pragma once
#include <stdafx.h>
#include "DataStruct.h"
#include "Resource/DeviceResource.h"
#include "Scene.h"
#include "Framework/RendererBase.h"
#include "Framework/GlobalResource.h"
#include "Pass/Pass.h"
#include "Resource/AFRResource/AFRBackendResource.h"
#include "Resource/AFRResource/AFRDisplayResource.h"
class AFRBeacon : public RendererBase {
public:
    AFRBeacon(uint width, uint height, std::wstring title, uint frameCount);
    AFRBeacon(const AFRBeacon &) = delete;
    AFRBeacon(AFRBeacon &&) = default;
    AFRBeacon &operator=(const AFRBeacon &) = delete;
    AFRBeacon &operator=(AFRBeacon &&) = default;
    ~AFRBeacon();

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
    void CreateRtv(HWND handle);
    bool CheckFeatureSupport();
    void CreateSharedFence();
    void ResetResourceState();
    void InitSceneCB();
    void InitPass();

    std::tuple<AFRResourceBase *, uint> GetCurrentContext() const;
    void IncrementContextIndex();

    void SetPass(AFRResourceBase *ctx,bool isDisplay);
    void SyncExecutePass(AFRResourceBase *ctx,bool isDisplay);


    CD3DX12_VIEWPORT mViewPort;
    CD3DX12_RECT mScissor;

    ComPtr<IDXGIFactory6> mFactory;

    const uint FrameCount;

    std::unique_ptr<Scene> mScene;

    std::shared_ptr<AFRDisplayResource> mDisplayResource;
    std::vector<std::shared_ptr<AFRBackendResource>> mBackendResource;
    std::vector<std::shared_ptr<AFRResourceBase>> mRenderContext;

    uint CurrentContextIndex;

    bool CrossAdapterTextureSupport = true;
};