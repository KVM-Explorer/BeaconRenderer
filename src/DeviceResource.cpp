#include "DeviceResource.h"
#include "D3DHelpler/ResourceRegister.h"

DeviceResource::DeviceResource(IDXGIFactory6 *factory, IDXGIAdapter1 *adapter, uint frameCount, Gpu type) :
    mDeviceType(type),
    mFrameCount(frameCount)
{
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CmdQueue)));

    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        infoQueue->GetMuteDebugOutput();
    }

    mResourceRegister = std::make_unique<ResourceRegister>(Device.Get());
}

DeviceResource::~DeviceResource()
{
    Signature.clear();
    PSO.clear();
    FR.clear();
    mResourceRegister.reset();
    SwapChain4.Reset();
    CmdQueue.Reset();
    Device.Reset();
}

void DeviceResource::CreateSwapChain(HWND handle, uint width, uint height, IDXGIFactory6 *factory)
{
    ComPtr<IDXGISwapChain1> swapchain1;
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.Width = width;
    swapchainDesc.Height = height;
    swapchainDesc.BufferCount = mFrameCount;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.SampleDesc.Count = 1;

    ThrowIfFailed(factory->CreateSwapChainForHwnd(CmdQueue.Get(),
                                                  handle,
                                                  &swapchainDesc,
                                                  nullptr,
                                                  nullptr,
                                                  &swapchain1));
    ThrowIfFailed(swapchain1.As(&SwapChain4));
}

HANDLE DeviceResource::InitFrameResource(uint width, uint height, uint frameIndex, HANDLE fenceHandle)
{
    CrossFrameResource resource(mResourceRegister.get(), Device.Get());
    if (mDeviceType == Gpu::Discrete) {
        resource.InitByMainGpu(Device.Get(), width, height);
    }

    if (mDeviceType == Gpu::Integrated) {
        ComPtr<ID3D12Resource> swapChainBuffer;
        SwapChain4->GetBuffer(frameIndex, IID_PPV_ARGS(&swapChainBuffer));
        resource.InitByAuxGpu(Device.Get(), swapChainBuffer.Get(), fenceHandle);
    }

    FR.push_back(std::move(resource));

    if (mDeviceType == Gpu::Discrete) return FR.back().SharedFenceHandle;
    return nullptr;
}

void DeviceResource::CreateRTV(uint width, uint height)
{
    for (uint i = 0; i < mFrameCount; i++) {
        if (mDeviceType == Gpu::Integrated) {
            ComPtr<ID3D12Resource> buffer;
            ThrowIfFailed(SwapChain4->GetBuffer(i, IID_PPV_ARGS(&buffer)));
            FR.at(i).CreateAuxRenderTarget(Device.Get(), buffer.Get());
        } else {
            FR.at(i).CreateMainRenderTarget(Device.Get(), width, height);
        }
    }
}
