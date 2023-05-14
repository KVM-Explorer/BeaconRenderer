#include "DisplayResource.h"

DisplayResource::DisplayResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount) :
    mFrameCount(frameCount)
{
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&DirectQueue)));

    mRTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDSVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCBVSRVUAVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        infoQueue->GetMuteDebugOutput();
    }

    mResourceRegister = std::make_unique<ResourceRegister>(Device.Get());
}

DisplayResource::~DisplayResource()
{
    mCopyHeaps.clear();
    for (auto &deviceFrames : mSFR) deviceFrames.clear();
    mSFR.clear();
    mQuadIB = nullptr;
    mQuadVB = nullptr;
    mResourceRegister.reset();
    DirectQueue.Reset();
    Device.Reset();
}

void DisplayResource::CreateSwapChain(IDXGIFactory6 *factory, HWND handle, uint width, uint height, size_t backendCount)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = backendCount * mFrameCount; // Swap chain uses double-buffering to minimize latency.
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

std::vector<HANDLE> DisplayResource::CreateRenderTargets(uint width, uint height, size_t backendCount)
{
    std::vector<HANDLE> handles;
    auto swapBufferCount = backendCount * mFrameCount;

    for (uint i = 0; i < backendCount; i++) {
        std::vector<StageFrameResource> deviceFrames;
        for (uint j = 0; j < mFrameCount; j++) {
            ComPtr<ID3D12Resource> backBuffer;
            uint index = i + j * backendCount;
            SwapChain->GetBuffer(index, IID_PPV_ARGS(&backBuffer));
            StageFrameResource frameResource(Device.Get(), mResourceRegister.get());
            HANDLE handle = frameResource.CreateLightCopyTexture(Device.Get(), width, height);
            frameResource.CreateSobelBuffer(Device.Get(), width, height);
            frameResource.CreateSwapChain(backBuffer.Get());
            deviceFrames.push_back(std::move(frameResource));
            handles.push_back(handle);
        }
        mSFR.push_back(std::move(deviceFrames));
    }
    return handles;
}

std::vector<HANDLE> DisplayResource::CreateCompatibleRenderTargets(uint width, uint height, size_t backendCount)
{
    std::vector<HANDLE> handles = CreateCopyHeap(width, height, backendCount);
    auto swapBufferCount = backendCount * mFrameCount;

    for (uint i = 0; i < backendCount; i++) {
        std::vector<StageFrameResource> deviceFrames;
        for (uint j = 0; j < mFrameCount; j++) {
            ComPtr<ID3D12Resource> backBuffer;
            uint index = i + j * backendCount;
            SwapChain->GetBuffer(index, IID_PPV_ARGS(&backBuffer));
            StageFrameResource frameResource(Device.Get(), mResourceRegister.get());
            frameResource.CreateLightCopyHeapBuffer(Device.Get(), mCopyHeaps[i].Get(), j, width, height, D3D12_RESOURCE_STATE_COPY_SOURCE);
            frameResource.CreateLightCopyHeapTexture(Device.Get(), width, height);
            frameResource.CreateSobelBuffer(Device.Get(), width, height);
            frameResource.CreateSwapChain(backBuffer.Get());
            deviceFrames.push_back(std::move(frameResource));
        }
        mSFR.push_back(std::move(deviceFrames));
    }
    return handles;
}

std::vector<HANDLE> DisplayResource::CreateSharedFence(size_t backendCount)
{
    std::vector<HANDLE> handles;
    for (uint i = 0; i < backendCount; i++) {
        for (uint j = 0; j < mFrameCount; j++) {
            auto &frameResource = mSFR[i][j];
            HANDLE handle = frameResource.CreateSharedFence(Device.Get());
            handles.push_back(handle);
        }
    }
    return handles;
}

std::tuple<StageFrameResource *, uint> DisplayResource::GetCurrentFrame(uint backendIndex, Stage stage, uint currentBackendIndex)
{
    switch (stage) {
    case Stage::PostProcess: {
        auto index = (currentBackendIndex - 2 + mFrameCount) % mFrameCount;
        return {&(mSFR[backendIndex][index]), index};
    }
    default:
        throw std::runtime_error("Invalid stage");
    }
}

void DisplayResource::CreateScreenQuadView()
{
    ScreenQuadVBView.BufferLocation = mQuadVB->resource()->GetGPUVirtualAddress();
    ScreenQuadVBView.StrideInBytes = sizeof(ModelVertex);
    ScreenQuadVBView.SizeInBytes = sizeof(ModelVertex) * 4;

    ScreenQuadIBView.BufferLocation = mQuadIB->resource()->GetGPUVirtualAddress();
    ScreenQuadIBView.Format = DXGI_FORMAT_R16_UINT;
    ScreenQuadIBView.SizeInBytes = sizeof(uint16_t) * 6;
}

std::vector<HANDLE> DisplayResource::CreateCopyHeap(uint width, uint height, size_t backendCount)
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