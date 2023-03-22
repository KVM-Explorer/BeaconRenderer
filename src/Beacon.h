#pragma once
#include "Framework/RendererBase.h"
#include <stdafx.h>
#include "Scene.h"
#include "Framework/ImguiManager.h"
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

    void OnInit(std::unique_ptr<ImguiManager> &guiContext) override;
    void OnKeyDown(byte key) override;
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnDestory() override;

private:
    void CreateDevice(HWND handle);
    void CreateCommandResource();
    void CreateSwapChain(HWND handle);
    void CreateFence();
    void LoadScene();
    void SyncTask();

private:
    CD3DX12_VIEWPORT mViewPort;
    CD3DX12_RECT mScissor;
    const uint mFrameCount = 3;
    uint64 mFenceValue = 0;

    ComPtr<ID3D12Device> mDevice;
    ComPtr<IDXGIFactory6> mFactory;
    ComPtr<IDXGIAdapter1> mDeviceAdapter;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    ComPtr<IDXGISwapChain4> mSwapChain;
    ComPtr<ID3D12Fence> mFence;
    std::unique_ptr<Scene> mScene;
    std::unique_ptr<ImguiManager> mGUI;
};
