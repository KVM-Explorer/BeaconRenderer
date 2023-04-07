#pragma once
#include "Framework/RendererBase.h"
#include <stdafx.h>
#include "Scene.h"
#include "Framework/ImguiManager.h"
#include "Framework/GlobalResource.h"
#include "Tools/D3D12GpuTimer.h"
#include "SobelFilter.h"
#include "ScreenQuad.h"
#include "FrameResource.h"

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
    void CreateCommandResource();
    std::vector<D3D12_INPUT_ELEMENT_DESC> CreateInputLayout();
    void CompileShaders();
    void CreateRTV(ID3D12Device *device, IDXGISwapChain4 *swapchain, uint frameCount);
    void CreateSwapChain(HWND handle);
    void CreateFence();
    void LoadScene();

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
    std::unique_ptr<SobelFilter> mPostProcesser;
    std::unique_ptr<DescriptorHeap> mRTVDescriptorHeap;
    std::vector<ComPtr<ID3D12Resource>> mRTVBuffer;
    std::vector<uint> mMediaRTVBuffer;
    std::vector<uint> mMediaSrvIndex;
    std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayout;

    // Multi-Pass
    std::unique_ptr<DeferredRendering> mDeferredRendering;
    std::unique_ptr<ScreenQuad> mQuadPass;
    std::array<FrameResource,mFrameCount> mFR;
};
