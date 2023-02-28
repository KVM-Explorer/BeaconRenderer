#pragma once
#include "Framework/RendererBase.h"
#include <stdafx.h>
class Beacon : public RendererBase {
public:
    Beacon(uint width, uint height, std::wstring title);
    Beacon(const Beacon &) = delete;
    Beacon(Beacon &&) = default;
    Beacon &operator=(const Beacon &) = delete;
    Beacon &operator=(Beacon &&) = default;

    void OnUpdate() override;
    void OnRender() override;

    void OnInit() override;
    void OnKeyDown(byte key) override;
    void OnMouseDown() override;
    void OnDestory() override;

private:
    CD3DX12_VIEWPORT mViewPort;
    CD3DX12_RECT mScissor;

    ComPtr<ID3D12Device> mDevice;
    ComPtr<IDXGIFactory6> mFactory;
    ComPtr<IDXGIAdapter1> mDeviceAdapter;
    ComPtr<ID3D12CommandList> mCommandList;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    ComPtr<IDXGISwapChain4> mSwapChain;
};
