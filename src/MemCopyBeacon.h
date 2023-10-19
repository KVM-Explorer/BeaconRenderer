#pragma once
#include <stdafx.h>
#include "D3DHelpler/ResourceRegister.h"
#include "Scene.h"
#include "Framework/RendererBase.h"
#include "Framework/GlobalResource.h"
#include "Resource/DeviceResource.h"
#include "Pass/Pass.h"

class MemCopyBeacon : public RendererBase {
public:
    MemCopyBeacon(uint width, uint height, std::wstring title);
    MemCopyBeacon(const MemCopyBeacon &) = delete;
    MemCopyBeacon(MemCopyBeacon &&) = default;
    MemCopyBeacon &operator=(const MemCopyBeacon &) = delete;
    MemCopyBeacon &operator=(MemCopyBeacon &&) = default;
    ~MemCopyBeacon();

    void OnUpdate() override;
    void OnRender() override;

    void OnInit() override;
    void OnKeyDown(byte key) override;
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnDestory() override;

private:
    int GetCurrentBackBuffer();
    void CreateDeviceResource(HWND handle);
    void CreateFrameResource();
    void CompileShaders();
    void CreateSignature2PSO();
    void CreatePass();
    void LoadScene();
    void CreateQuad();
    void CreateRtv();
    void CreateCopyResource();
    void SetGpuTimers();

    void SetPass(uint frameIndex);
    void ExecutePass(uint frameIndex);

    void RenderStage(CrossFrameResource &ctx, D3D12_GPU_VIRTUAL_ADDRESS constantAddr);
    void MemCopyStage(CrossFrameResource &ctx);
    void DisplayStage(CrossFrameResource &ctx);
    void DeviceHostCopyStage(CrossFrameResource &ctx);
    void HostDeviceCopyStage(CrossFrameResource &resource,CrossFrameResource &ctx);

private:
    CD3DX12_VIEWPORT mViewPort;
    CD3DX12_RECT mScissor;
    static const uint mFrameCount = 3;
    int mCurrentBackBuffer = 0;

    ComPtr<IDXGIFactory6> mFactory;
    std::unordered_map<Gpu, std::unique_ptr<DeviceResource>> mDResource;

    std::unique_ptr<Scene> mScene;

    // Temporary Vertex Buffer
    std::unique_ptr<UploadBuffer<ModelVertex>> mIGpuQuadVB;
    D3D12_VERTEX_BUFFER_VIEW mIGpuQuadVBView;

    // Pass
    std::unique_ptr<GBufferPass> mGBufferPass;
    std::unique_ptr<LightPass> mLightPass;
    std::unique_ptr<SobelPass> mSobelPass;
    std::unique_ptr<MixQuadPass> mQuadPass;
};