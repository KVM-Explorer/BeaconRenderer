#include "AFRDisplayResource.h"

AFRDisplayResource::AFRDisplayResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount, std::wstring name) :
    AFRResourceBase(factory, adapter, frameCount, name)
{
}

AFRDisplayResource::~AFRDisplayResource()
{
    SwapChain.Reset();
    mQuadVB = nullptr;
    mQuadIB = nullptr;
    mCopyHeaps.clear();
    mSharedFR.clear();
    Release();
}

std::vector<HANDLE> AFRDisplayResource::CreateRenderTargets(uint width, uint height, size_t backendCount)
{
    std::vector<HANDLE> handles;
    auto swapBufferCount = (backendCount + 1) * mFrameCount; // Display Device has the same render resource

    for (uint i = 0; i < backendCount; i++) {
        std::vector<StageFrameResource> deviceFrames;
        for (uint j = 0; j < mFrameCount; j++) {
            ComPtr<ID3D12Resource> backBuffer;
            uint index = i + j * (backendCount + 1);
            SwapChain->GetBuffer(index, IID_PPV_ARGS(&backBuffer));
            StageFrameResource frameResource(Device.Get(), mResourceRegister.get());
            HANDLE handle = frameResource.CreateLightCopyTexture(Device.Get(), width, height);
            frameResource.CreateSwapChain(backBuffer.Get());
            deviceFrames.push_back(std::move(frameResource));
            handles.push_back(handle);
        }
        mSharedFR.push_back(std::move(deviceFrames));
    }

    // Display Render Targets
    for (uint i = 0; i < mFrameCount; i++) {
        ComPtr<ID3D12Resource> backBuffer;
        uint index = i * (backendCount + 1);
        SwapChain->GetBuffer(index, IID_PPV_ARGS(&backBuffer));
        StageFrameResource frameResource(Device.Get(), mResourceRegister.get());
        CreateRenderTarget(&frameResource, width, height);
        frameResource.CreateSwapChain(backBuffer.Get());
        mSFR.push_back(std::move(frameResource));
    }

    return handles;
}

std::vector<HANDLE> AFRDisplayResource::CreateSharedFence(size_t backendCount)
{
    std::vector<HANDLE> handles;
    for (uint i = 0; i < backendCount; i++) {
        for (uint j = 0; j < mFrameCount; j++) {
            auto &frameResource = mSharedFR[i][j];
            HANDLE handle = frameResource.CreateSharedFence(Device.Get());
            handles.push_back(handle);
        }
    }
    return handles;
}

void AFRDisplayResource::CreateSwapChain(IDXGIFactory6 *factory, HWND handle, uint width, uint height, size_t backendCount)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = (backendCount + 1) * mFrameCount; // Swap chain uses double-buffering to minimize latency.
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(DirectQueue.Get(), handle, &swapChainDesc, nullptr, nullptr, &swapChain));
    ThrowIfFailed(factory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain.As(&SwapChain));
}

void AFRDisplayResource::CreateScreenQuadView()
{
    ScreenQuadVBView.BufferLocation = mQuadVB->resource()->GetGPUVirtualAddress();
    ScreenQuadVBView.StrideInBytes = sizeof(ModelVertex);
    ScreenQuadVBView.SizeInBytes = sizeof(ModelVertex) * 4;

    ScreenQuadIBView.BufferLocation = mQuadIB->resource()->GetGPUVirtualAddress();
    ScreenQuadIBView.Format = DXGI_FORMAT_R32_UINT;
    ScreenQuadIBView.SizeInBytes = sizeof(uint) * 6;
}