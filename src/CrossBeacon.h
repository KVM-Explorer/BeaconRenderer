#pragma once
#include <stdafx.h>
#include "D3DHelpler/ResourceRegister.h"
#include "CrossFrameResource.h"
#include "Scene.h"
#include "Framework/RendererBase.h"
#include "Framework/GlobalResource.h"

class CrossBeacon :public RendererBase {
public:
    CrossBeacon(uint width, uint height, std::wstring title);
    CrossBeacon(const CrossBeacon &) = delete;
    CrossBeacon(CrossBeacon &&) = default;
    CrossBeacon &operator=(const CrossBeacon &) = delete;
    CrossBeacon &operator=(CrossBeacon &&) = default;
    ~CrossBeacon();

    void OnUpdate() override;
    void OnRender()     override;

    void OnInit() override;
    void OnKeyDown(byte key) override;
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnDestory() override;

private:
    int GetCurrentBackBuffer();
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
    int mCurrentBackBuffer = 0;

    ComPtr<ID3D12Device> mDevice;
    std::vector<ComPtr<ID3D12Device>> mCrossDevice;
    ComPtr<IDXGIFactory6> mFactory;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<IDXGISwapChain4> mSwapChain;
    std::unique_ptr<Scene> mScene;
    std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayout;

    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSO;
    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> mSignature;

    std::vector<std::unique_ptr<ResourceRegister>> mCrossResourceRegister;
    std::vector<std::vector<CrossFrameResource>> mCFR;
    uint MainGpu; // Main GPU
    uint AuxGpu;  // Aux GPU
};