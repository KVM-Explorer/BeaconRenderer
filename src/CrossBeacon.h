#pragma once
#include <stdafx.h>
#include "D3DHelpler/ResourceRegister.h"
#include "CrossFrameResource.h"
#include "Scene.h"
#include "Framework/RendererBase.h"
#include "Framework/GlobalResource.h"
#include "DeviceResource.h"
#include "Pass/Pass.h"

class CrossBeacon : public RendererBase {
public:
    CrossBeacon(uint width, uint height, std::wstring title);
    CrossBeacon(const CrossBeacon &) = delete;
    CrossBeacon(CrossBeacon &&) = default;
    CrossBeacon &operator=(const CrossBeacon &) = delete;
    CrossBeacon &operator=(CrossBeacon &&) = default;
    ~CrossBeacon();

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

    void SetPass(uint frameIndex);
    void ExecutePass(uint frameIndex);

private:
    CD3DX12_VIEWPORT mViewPort;
    CD3DX12_RECT mScissor;
    static const uint mFrameCount = 3;
    int mCurrentBackBuffer = 0;

    ComPtr<IDXGIFactory6> mFactory;
    std::unordered_map<Gpu, std::unique_ptr<DeviceResource>> mDResource;

    std::unique_ptr<Scene> mScene;
    std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayout;

    // Temporary Vertex Buffer
    std::unique_ptr<UploadBuffer<ModelVertex>> mIGpuQuadVB;
    D3D12_VERTEX_BUFFER_VIEW mIGpuQuadVBView;

    
    // Pass
    std::unique_ptr<GBufferPass> mGBufferPass;
    std::unique_ptr<LightPass> mLightPass;
    std::unique_ptr<SobelPass> mSobelPass;
    std::unique_ptr<QuadPass> mQuadPass;
};