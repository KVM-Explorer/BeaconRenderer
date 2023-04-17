#include "CrossBeacon.h"
#include "Framework/Application.h"
#include <pix3/pix3.h>
#include <iostream>
#include "Pass/Pass.h"
#include "GpuEntryLayout.h"

CrossBeacon::CrossBeacon(uint width, uint height, std::wstring title) :
    RendererBase(width, height, title),
    mViewPort(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
{
}

CrossBeacon::~CrossBeacon()
{
    mInputLayout.clear();
    mScene = nullptr;
    mDResource.clear();
    mFactory = nullptr;
}

void CrossBeacon::OnUpdate()
{
    uint frameIndex = mCurrentBackBuffer;

    auto &localFrameResource = mDResource[Gpu::Discrete]->FR.at(frameIndex).LocalResource;

    localFrameResource->Sync();

    mScene->UpdateCamera();
    mScene->UpdateSceneConstant(localFrameResource->SceneConstant.get());
    mScene->UpdateEntityConstant(localFrameResource->EntityConstant.get());
    mScene->UpdateLightConstant(localFrameResource->LightConstant.get());
    mScene->UpdateMaterialConstant(localFrameResource->MaterialConstant.get());
}

void CrossBeacon::OnRender()
{
    // TODO render scene
}

void CrossBeacon::OnInit()
{
    HWND handle = Application::GetHandle();
    CreateDeviceResource(handle);
    CompileShaders();
    CreateSignature2PSO();

    GResource::GUIManager->Init(mDResource[Gpu::Integrated]->Device.Get());

    CreateFrameResource();

    mDResource[Gpu::Discrete]->CreateRTV(mWidth, mHeight);
    mDResource[Gpu::Discrete]->CreateRTV(mWidth, mHeight);

    // CreatePass
    mDResource[Gpu::Discrete]->FR.at(0).Reset3D();
    CreatePass();

    LoadScene();

    // Sync
    mDResource[Gpu::Discrete]->FR.at(0).Signal3D(mDResource[Gpu::Discrete]->CmdQueue.Get());
    mDResource[Gpu::Discrete]->FR.at(0).Sync3D();
}

void CrossBeacon::OnKeyDown(byte key)
{}

void CrossBeacon::OnMouseDown(WPARAM btnState, int x, int y)
{
    using DirectX::XMConvertToRadians;
    if ((btnState & MK_LBUTTON) != 0) {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - MouseLastPosition.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - MouseLastPosition.y));
        mScene->UpdateMouse(dx, dy);
    }
    MouseLastPosition.x = x;
    MouseLastPosition.y = y;
}

void CrossBeacon::OnDestory()
{}

int CrossBeacon::GetCurrentBackBuffer()
{
    return mCurrentBackBuffer = (mCurrentBackBuffer + 1) % 3;
}

void CrossBeacon::CreateDeviceResource(HWND handle)
{
    ComPtr<ID3D12Debug3> debugController;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput> output;
    UINT dxgiFactoryFlags = 0;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));
    ThrowIfFailed(mFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES));

    DXGI_ADAPTER_DESC1 adapterDesc = {};
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != mFactory->EnumAdapters1(i, &adapter); i++) {
        adapter->GetDesc1(&adapterDesc);
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) continue;
        std::wstring str = adapterDesc.Description;

        auto hr = adapter->EnumOutputs(0, &output);
        if (SUCCEEDED(hr) && output != nullptr) {
            auto outputStr = std::format(L"iGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
            OutputDebugStringW(outputStr.c_str());
            mDResource[Gpu::Integrated] = std::make_unique<DeviceResource>(mFactory.Get(), adapter.Get(), mFrameCount, Gpu::Integrated);
        } else {
            auto outputStr = std::format(L"dGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
            OutputDebugStringW(outputStr.c_str());
            mDResource[Gpu::Discrete] = std::make_unique<DeviceResource>(mFactory.Get(), adapter.Get(), mFrameCount, Gpu::Discrete);
        }
    }

    mDResource[Gpu::Integrated]->CreateSwapChain(handle, GetWidth(), GetHeight(), mFactory.Get());
}

void CrossBeacon::CreateFrameResource()
{
    for (uint i = 0; i < mFrameCount; i++) {
        auto fenceHandle = mDResource[Gpu::Discrete]->InitFrameResource(GetWidth(), GetHeight(), i, nullptr);
        mDResource[Gpu::Integrated]->InitFrameResource(GetWidth(), GetHeight(), i, fenceHandle);
    }
}

void CrossBeacon::CompileShaders()
{
    // MainGpu
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/GBuffer.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["GBufferVS"], &error));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/GBuffer.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["GBufferPS"], &error));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/LightingPass.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["LightPassVS"], &error));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/LightingPass.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["LightPassPS"], &error));

    // AuxGpu
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/PostProcess.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "SobelMain", "cs_5_1", compileFlags, 0, &GResource::Shaders["SobelCS"], &error));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/ScreenQuad.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["ScreenQuadPS"], &error));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/ScreenQuad.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["ScreenQuadVS"], &error));

    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
}

void CrossBeacon::CreateSignature2PSO()
{
    GResource::InputLayout = GpuEntryLayout::CreateInputLayout();

    // MAinGpu
    mDResource[Gpu::Discrete]->Signature["Graphic"] =
        GpuEntryLayout::CreateRenderSignature(mDResource[Gpu::Discrete]->Device.Get(),
                                              GBufferPass::GetTargetCount());
    mDResource[Gpu::Discrete]->PSO["GBuffer"] =
        GpuEntryLayout::CreateGBufferPassPSO(mDResource[Gpu::Discrete]->Device.Get(),
                                             mDResource[Gpu::Discrete]->Signature["Graphic"].Get(),
                                             GBufferPass::GetTargetFormat(),
                                             GBufferPass::GetDepthFormat());
    mDResource[Gpu::Discrete]->PSO["LightPass"] =
        GpuEntryLayout::CreateLightPassPSO(mDResource[Gpu::Discrete]->Device.Get(),
                                           mDResource[Gpu::Discrete]->Signature["Graphic"].Get());

    // AuxGpu
    mDResource[Gpu::Integrated]->Signature["Graphic"] =
        GpuEntryLayout::CreateRenderSignature(mDResource[Gpu::Integrated]->Device.Get(),
                                              GBufferPass::GetTargetCount());
    mDResource[Gpu::Integrated]->Signature["Compute"] =
        GpuEntryLayout::CreateComputeSignature(mDResource[Gpu::Integrated]->Device.Get());
    mDResource[Gpu::Integrated]->PSO["SobelPass"] =
        GpuEntryLayout::CreateSobelPSO(mDResource[Gpu::Integrated]->Device.Get(),
                                       mDResource[Gpu::Integrated]->Signature["Compute"].Get());
    mDResource[Gpu::Integrated]->PSO["QuadPass"] =
        GpuEntryLayout::CreateQuadPassPSO(mDResource[Gpu::Integrated]->Device.Get(),
                                          mDResource[Gpu::Integrated]->Signature["Graphic"].Get());
}

void CrossBeacon::CreatePass()
{
    // TODO create pass
}

void CrossBeacon::LoadScene()
{
    // TODO add read config file
    std::string Path = "./Assets";
    auto scene = std::make_unique<Scene>(Path, "witch");
    scene->Init(mDResource[Gpu::Discrete]->Device.Get(),
                mDResource[Gpu::Discrete]->FR.at(0).LocalResource->CmdList.Get(),
                mDResource[Gpu::Discrete]->mResourceRegister->SrvCbvUavDescriptorHeap.get());

    mScene = std::move(scene);
    for (auto &item : mDResource[Gpu::Discrete]->FR) {
        item.CreateConstantBuffer(mDResource[Gpu::Discrete]->Device.Get(), mScene->GetEntityCount(), 1, mScene->GetMaterialCount());
    }
}

void CrossBeacon::SetPass(uint frameIndex)
{
    // TODO set pass
}

void CrossBeacon::ExecutePass(uint frameIndex)
{
    // TODO execute pass
}
