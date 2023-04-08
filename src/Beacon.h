#pragma once
#include "Framework/RendererBase.h"
#include <stdafx.h>
#include "Scene.h"
#include "Framework/ImguiManager.h"
#include "Framework/GlobalResource.h"
#include "Tools/D3D12GpuTimer.h"
#include "FrameResource.h"
#include "Pass/Pass.h"

class Beacon : public RendererBase {
public:
    Beacon(uint width, uint height, std::wstring title);
    Beacon(const Beacon &) = delete;
    Beacon(Beacon &&) = default;
    Beacon &operator=(const Beacon &) = delete;
    Beacon &operator=(Beacon &&) = default;
    ~Beacon();

    void OnUpdate() override;
    void OnRender() override;

    void OnInit() override;
    void OnKeyDown(byte key) override;
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnDestory() override;

private:
    void CreateDevice(HWND handle);
    void CreateCommandQueue();
    void CompileShaders();
    void CreateRTV(ID3D12Device *device, IDXGISwapChain4 *swapchain, uint frameCount);
    void CreateSwapChain(HWND handle);
    void CreateSignature2PSO();
    void CreatePass();
    void LoadScene();

    void SetPass(uint frameIndex);
    void ExecutePass(uint frameIndex);

private:
    CD3DX12_VIEWPORT mViewPort;
    CD3DX12_RECT mScissor;
    static const uint mFrameCount = 3;

    ComPtr<ID3D12Device> mDevice;
    ComPtr<IDXGIFactory6> mFactory;
    ComPtr<IDXGIAdapter1> mDeviceAdapter;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<IDXGISwapChain4> mSwapChain;
    std::unique_ptr<Scene> mScene;
    std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayout;

    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSO;
    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> mSignature;

    // Multi-Pass
    std::array<FrameResource, mFrameCount> mFR;
    std::unique_ptr<GBufferPass> mGBufferPass;
    std::unique_ptr<LightPass> mLightPass;
    std::unique_ptr<SobelPass> mSobelPass;
    std::unique_ptr<QuadPass> mQuadPass;
};
