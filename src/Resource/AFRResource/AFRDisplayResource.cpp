#include "AFRDisplayResource.h"

AFRDisplayResource::AFRDisplayResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount, std::wstring name) :
    AFRResourceBase(factory, adapter, frameCount, name)
{
}

AFRDisplayResource::~AFRDisplayResource()
{
    SwapChain.Reset();
    mCopyHeaps.clear();
    mSharedFR.clear();
    Release();
}

std::vector<HANDLE> AFRDisplayResource::CreateRenderTargets(uint width, uint height, size_t backendCount)
{
    std::vector<HANDLE> handles = CreateCopyHeap(width, height, backendCount);
    auto swapBufferCount = (backendCount + 1) * mFrameCount; // Display Device has the same render resource

    for (uint i = 0; i < backendCount; i++) {
        std::vector<StageFrameResource> deviceFrames;
        for (uint j = 0; j < mFrameCount; j++) {
            ComPtr<ID3D12Resource> backBuffer;
            uint index = i + j * (backendCount + 1);
            SwapChain->GetBuffer(index, IID_PPV_ARGS(&backBuffer));
            StageFrameResource frameResource(Device.Get(), mResourceRegister.get());
            frameResource.CreateLightCopyHeapBuffer(Device.Get(), mCopyHeaps[i].Get(), j, width, height, D3D12_RESOURCE_STATE_COPY_SOURCE);
            frameResource.CreateLightCopyTexture(Device.Get(), width, height);
            frameResource.CreateSwapChain(backBuffer.Get());
            deviceFrames.push_back(std::move(frameResource));
        }
        mSharedFR.push_back(std::move(deviceFrames));
    }

    // Display Render Targets
    for (uint i = 0; i < mFrameCount; i++) {
        ComPtr<ID3D12Resource> backBuffer;
        uint index = (i + 1) * (backendCount + 1) -1;
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

std::vector<HANDLE> AFRDisplayResource::CreateCopyHeap(uint width, uint height, size_t backendCount)
{
    auto textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, 1);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts;
    Device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &layouts, nullptr, nullptr, nullptr);

    uint64 bufferSize = UpperMemorySize(layouts.Footprint.RowPitch * layouts.Footprint.Height, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER);

    auto heapDesc = CD3DX12_HEAP_DESC(bufferSize * mFrameCount, D3D12_HEAP_TYPE_DEFAULT, 0, D3D12_HEAP_FLAG_SHARED | D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER);

    std::vector<HANDLE> handles;
    for (uint i = 0; i < backendCount; i++) {
        ComPtr<ID3D12Heap> heap;
        HANDLE handle;
        ThrowIfFailed(Device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));
        ThrowIfFailed(Device->CreateSharedHandle(heap.Get(), nullptr, GENERIC_ALL, nullptr, &handle));
        mCopyHeaps.push_back(heap);
        handles.push_back(handle);
    }
    return handles;
}