#include "MemCopyBeacon.h"
#include "Framework/Application.h"
#include <pix3/pix3.h>
#include <iostream>
#include "Pass/Pass.h"
#include "GpuEntryLayout.h"
#include <future>

MemCopyBeacon::MemCopyBeacon(uint width, uint height, std::wstring title) :
    RendererBase(width, height, title),
    mViewPort(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
{
}

MemCopyBeacon::~MemCopyBeacon()
{
}

void MemCopyBeacon::OnUpdate()
{
    uint frameIndex = mCurrentBackBuffer;

    auto &dFR = mDResource[Gpu::Discrete]->FR.at(frameIndex);
    auto &iFR = mDResource[Gpu::Integrated]->FR.at(frameIndex);

    // dCrossFrameResource.Sync3D();

    mScene->UpdateCamera();
    mScene->UpdateSceneConstant(dFR.SceneConstant.get());
    mScene->UpdateEntityConstant(dFR.EntityConstant.get());
    mScene->UpdateLightConstant(dFR.LightConstant.get());
    mScene->UpdateMaterialConstant(dFR.MaterialConstant.get());
}

void MemCopyBeacon::OnRender()
{
    uint frameIndex = mCurrentBackBuffer;
    GResource::CPUTimerManager->UpdateTimer("RenderTime");
    GResource::CPUTimerManager->UpdateAvgTimer("AvgTime1000");

    // update gui fps
    TimePoint now = std::chrono::high_resolution_clock::now();
    static uint fpsCount = 0;
    fpsCount++;
    auto duration = GResource::CPUTimerManager->GetStaticTimeDuration("FPS", now);
    if (duration > 1000) {
        GResource::CPUTimerManager->SetStaticTime("FPS", now);
        GResource::GUIManager->State.FPSCount = fpsCount;
        fpsCount = 0;
    }

    GResource::CPUTimerManager->BeginTimer("DrawCall");
    ExecutePass(frameIndex);
    GResource::CPUTimerManager->EndTimer("DrawCall");

    GetCurrentBackBuffer();
}

void MemCopyBeacon::OnInit()
{
    HWND handle = Application::GetHandle();
    CreateDeviceResource(handle);

    CompileShaders();
    CreateSignature2PSO();

    GResource::GUIManager->Init(mDResource[Gpu::Integrated]->Device.Get());
    CreateFrameResource();
    SetGpuTimers();
    CreateRtv();

    // CreatePass
    mDResource[Gpu::Discrete]->FR.at(0).Reset3D();
    CreatePass();

    LoadScene();

    CreateCopyResource();

    // Sync
    mDResource[Gpu::Discrete]->FR.at(0).Signal3D(mDResource[Gpu::Discrete]->CmdQueue.Get());
    mDResource[Gpu::Discrete]->FR.at(0).Sync3D();

    mDResource[Gpu::Integrated]->FR.at(0).Reset3D();
    CreateQuad();
    mDResource[Gpu::Integrated]->FR.at(0).Signal3D(mDResource[Gpu::Integrated]->CmdQueue.Get());
    mDResource[Gpu::Integrated]->FR.at(0).Sync3D();
}

void MemCopyBeacon::OnKeyDown(byte key)
{}

void MemCopyBeacon::OnMouseDown(WPARAM btnState, int x, int y)
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

void MemCopyBeacon::OnDestory()
{
    for (auto &item : mDResource[Gpu::Discrete]->FR) {
        item.Sync3D();
        item.SyncCopy(item.SharedFenceValue);
    }
    for (auto &item : mDResource[Gpu::Integrated]->FR) {
        item.SignalOnly(mDResource[Gpu::Integrated]->CmdQueue.Get());
        item.Sync3D();
    }
    GResource::Desctory();
    mIGpuQuadVB.reset();
    mScene = nullptr;
    mDResource.clear();
    mFactory = nullptr;
}

int MemCopyBeacon::GetCurrentBackBuffer()
{
    return mCurrentBackBuffer = (mCurrentBackBuffer + 1) % 3;
}

void MemCopyBeacon::SetGpuTimers()
{
    // Display Gpu Timer
    GResource::GPUTimer = std::make_unique<D3D12GpuTimer>(mDResource[Gpu::Integrated]->Device.Get(),
                                                          mDResource[Gpu::Integrated]->CmdQueue.Get(), 1);
    GResource::GPUTimer->SetTimerName(0, "Stage3");

    // Backend Gpu Timer

    auto *device = mDResource[Gpu::Discrete]->Device.Get();
    auto *renderQueue = mDResource[Gpu::Discrete]->CmdQueue.Get();
    // mBackendResource[i]
    //     ->mRenderGTimer = std::make_unique<D3D12GpuTimer>(device, renderQueue, 1);
    // mBackendResource[i]->mRenderGTimer->SetTimerName(0, "GPU:" + id + "Stage1");
}

void MemCopyBeacon::CreateDeviceResource(HWND handle)
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

    DXGI_ADAPTER_DESC1 adapterDesc = {};
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != mFactory->EnumAdapters1(i, &adapter); i++) {
        adapter->GetDesc1(&adapterDesc);
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) continue;
        std::wstring str = adapterDesc.Description;

        auto hr = adapter->EnumOutputs(0, &output);
        // if (output != nullptr) continue;
        // if (SUCCEEDED(hr) && output != nullptr) {
        //     auto outputStr = std::format(L"iGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
        //     OutputDebugStringW(outputStr.c_str());
        //     mDResource[Gpu::Integrated] = std::make_unique<DeviceResource>(mFactory.Get(), adapter.Get(), mFrameCount, Gpu::Integrated);
        // } else {
        //     static int device_num = 0;
        //     if (device_num == 0) {
        //         auto outputStr = std::format(L"dGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
        //         OutputDebugStringW(outputStr.c_str());
        //         mDResource[Gpu::Discrete] = std::make_unique<DeviceResource>(mFactory.Get(), adapter.Get(), mFrameCount, Gpu::Discrete);
        //     }
        //     if (device_num == 1) {
        //         auto outputStr = std::format(L"iGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
        //         OutputDebugStringW(outputStr.c_str());
        //         mDResource[Gpu::Integrated] = std::make_unique<DeviceResource>(mFactory.Get(), adapter.Get(), mFrameCount, Gpu::Integrated);
        //     }
        //   device_num++;
         if (str.find(L"1060 6G") != std::string::npos) {
            static int id = 0;
            if (id == 1) {
                OutputDebugStringW(std::format(L"Found Display IGPU:  {}\n", str).c_str());
                mDResource[Gpu::Integrated] = std::make_unique<DeviceResource>(mFactory.Get(), adapter.Get(), mFrameCount, Gpu::Integrated);
            } else {
                auto outputStr = std::format(L"dGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
                OutputDebugStringW(outputStr.c_str());
                mDResource[Gpu::Discrete] = std::make_unique<DeviceResource>(mFactory.Get(), adapter.Get(), mFrameCount, Gpu::Discrete);
            }
            id++;
        }
        // if (id == 2) break;
        // }
    }
    mDResource[Gpu::Integrated]->CreateSwapChain(handle, GetWidth(), GetHeight(), mFactory.Get());
    ThrowIfFailed(mFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES));
}

void MemCopyBeacon::CreateFrameResource()
{
    for (uint i = 0; i < mFrameCount; i++) {
        auto fenceHandle = mDResource[Gpu::Discrete]->InitFrameResource(GetWidth(), GetHeight(), i, nullptr);
        mDResource[Gpu::Integrated]->InitFrameResource(GetWidth(), GetHeight(), i, fenceHandle);
    }
}

void MemCopyBeacon::CreateQuad()
{
    const std::vector<ModelVertex> vertices{{
        {{-1.0F, 1.0F, 0.0F}, {}, {0.0F, 0.0F}},
        {{1.0F, 1.0F, 0.0F}, {}, {1.0F, 0.0F}},
        {{-1.0F, -1.0F, 0.0F}, {}, {0.0F, 1.0F}},
        {{1.0F, -1.0F, 0.0F}, {}, {1.0F, 1.0F}},
    }};

    mIGpuQuadVB = std::make_unique<UploadBuffer<ModelVertex>>(mDResource[Gpu::Integrated]->Device.Get(), vertices.size(), false);
    mIGpuQuadVB->copyAllData(vertices.data(), vertices.size());
    mIGpuQuadVBView.BufferLocation = mIGpuQuadVB->resource()->GetGPUVirtualAddress();
    mIGpuQuadVBView.SizeInBytes = static_cast<UINT>(sizeof(ModelVertex) * vertices.size());
    mIGpuQuadVBView.StrideInBytes = sizeof(ModelVertex);
}

void MemCopyBeacon::CreateRtv()
{
    auto handle =
        mDResource[Gpu::Discrete]->CreateCopyHeap(GetWidth(), GetHeight(), mDResource[Gpu::Discrete]->Device.Get(), mFrameCount);
    mDResource[Gpu::Integrated]->OpenCopyHeapByHeandle(handle);
    for (uint i = 0; i < mFrameCount; i++) {
        auto &dFR = mDResource[Gpu::Discrete]->FR.at(i);
        auto &iFR = mDResource[Gpu::Integrated]->FR.at(i);
        ComPtr<ID3D12Resource> swapChainBuffer;
        mDResource[Gpu::Integrated]->SwapChain4->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer));

        auto *igpuHeap = mDResource[Gpu::Integrated]->mCopyHeap.Get();
        auto *dgpuHeap = mDResource[Gpu::Discrete]->mCopyHeap.Get();

        dFR.CreateMainRenderTarget(mDResource[Gpu::Discrete]->Device.Get(), GetWidth(), GetHeight(), dgpuHeap, i);
        iFR.CreateAuxRenderTarget(mDResource[Gpu::Integrated]->Device.Get(), swapChainBuffer.Get(), igpuHeap, i);
    }
}

void MemCopyBeacon::CompileShaders()
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
    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/ScreenQuad.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["ScreenQuadVS"], &error));

    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
}

void MemCopyBeacon::CreateSignature2PSO()
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
        GpuEntryLayout::CreateMixQuadPassPSO(mDResource[Gpu::Integrated]->Device.Get(),
                                             mDResource[Gpu::Integrated]->Signature["Graphic"].Get());
}

void MemCopyBeacon::CreatePass()
{
    mGBufferPass = std::make_unique<GBufferPass>(mDResource[Gpu::Discrete]->PSO["GBuffer"].Get(),
                                                 mDResource[Gpu::Discrete]->Signature["Graphic"].Get());

    mLightPass = std::make_unique<LightPass>(mDResource[Gpu::Discrete]->PSO["LightPass"].Get(),
                                             mDResource[Gpu::Discrete]->Signature["Graphic"].Get());

    mSobelPass = std::make_unique<SobelPass>(mDResource[Gpu::Integrated]->PSO["SobelPass"].Get(),
                                             mDResource[Gpu::Integrated]->Signature["Compute"].Get());

    mQuadPass = std::make_unique<MixQuadPass>(mDResource[Gpu::Integrated]->PSO["QuadPass"].Get(),
                                              mDResource[Gpu::Integrated]->Signature["Graphic"].Get());
}

void MemCopyBeacon::LoadScene()
{
    // TODO add read config file
    std::string path = GResource::config["Scene"]["ScenePath"].as<std::string>();
    std::string name = GResource::config["Scene"]["SceneName"].as<std::string>();
    auto scene = std::make_unique<Scene>(path, name);
    scene->Init(mDResource[Gpu::Discrete]->Device.Get(),
                mDResource[Gpu::Discrete]->FR.at(0).CmdList3D.Get(),
                mDResource[Gpu::Discrete]->mResourceRegister->SrvCbvUavDescriptorHeap.get());

    mScene = std::move(scene);
    for (auto &item : mDResource[Gpu::Discrete]->FR) {
        item.CreateConstantBuffer(mDResource[Gpu::Discrete]->Device.Get(), mScene->GetEntityCount(), 1, mScene->GetMaterialCount());
    }
}

void MemCopyBeacon::CreateCopyResource()
{
    for (auto &item : mDResource[Gpu::Discrete]->FR) {
        auto *resource = item.GetResource("LightTexture");
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
        auto desc = resource->GetDesc();

        mDResource[Gpu::Discrete]->Device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, nullptr, nullptr);
        item.mCopyFR->CreateReadBackResource(mDResource[Gpu::Discrete]->Device.Get(),
                                             layout.Footprint.Width,
                                             layout.Footprint.Height,
                                             layout.Footprint.RowPitch);
        item.mCopyFR->CreateUploadResource(mDResource[Gpu::Integrated]->Device.Get(),
                                           layout.Footprint.Width,
                                           layout.Footprint.Height,
                                           layout.Footprint.RowPitch);
    }
}

void MemCopyBeacon::SetPass(uint frameIndex)
{
    auto &dFR = mDResource[Gpu::Discrete]->FR.at((frameIndex + 2) % mFrameCount);
    auto &iFR = mDResource[Gpu::Integrated]->FR.at(frameIndex);

    // ===================== GBuffer Pass =====================
    std::vector<ID3D12Resource *> gbuffer;
    for (uint i = 0; i < GBufferPass::GetTargetCount(); i++) {
        std::string index = "GBuffer" + std::to_string(i);
        gbuffer.push_back(dFR.GetResource(index));
    }
    auto rtvHandle = dFR.GetRtv("GBuffer0");
    auto dsvHandle = dFR.GetDsv("Depth");

    mGBufferPass->SetRenderTarget(gbuffer, rtvHandle);
    mGBufferPass->SetDepthBuffer(dFR.GetResource("Depth"), dsvHandle);
    mGBufferPass->SetRTVDescriptorSize(mDResource[Gpu::Discrete]->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

    // ===================== Light Pass =====================
    mLightPass->SetRenderTarget(dFR.GetResource("LightTexture"), dFR.GetRtv("LightTexture"));
    mLightPass->SetGBuffer(dFR.GetSrvCbvUavHeap(), dFR.GetSrvCbvUav("GBuffer0"));
    mLightPass->SetTexture(dFR.GetSrvCbvUavHeap(), dFR.GetSrvBase());

    // ===================== Sobel Pass =====================
    mSobelPass->SetInput(iFR.GetSrvCbvUav("LightTexture"));
    mSobelPass->SetSrvHeap(iFR.GetSrvCbvUavHeap());
    mSobelPass->SetTarget(iFR.GetResource("ScreenTexture2"), iFR.GetSrvCbvUav("ScreenTexture2"));

    // ===================== Quad Pass =====================
    mQuadPass->SetTarget(iFR.GetResource("SwapChain"), iFR.GetRtv("SwapChain"));
    mQuadPass->SetRenderType(QuadShader::MixQuad);
    mQuadPass->SetSrvHandle(iFR.GetSrvCbvUav("LightTexture")); // LightCopy Sobel SwapChain
}

void MemCopyBeacon::ExecutePass(uint frameIndex)
{
    auto &nnextFrame = mDResource[Gpu::Discrete]->FR.at((frameIndex + 2) % mFrameCount);
    auto &nextFrame = mDResource[Gpu::Discrete]->FR.at((frameIndex + 1) % mFrameCount);
    auto &nextFrameIntegrated = mDResource[Gpu::Integrated]->FR.at((frameIndex + 1) % mFrameCount);
    auto &currentFrame = mDResource[Gpu::Integrated]->FR.at(frameIndex);

    SetPass(frameIndex);

    auto dConstantAddr = nnextFrame.EntityConstant->resource()->GetGPUVirtualAddress();

    auto renderFuture = std::async(std::launch::async, [&]() {
        RenderStage(nnextFrame, dConstantAddr);
    });

    auto displayFuture = std::async(std::launch::async, [&]() {
        DisplayStage(currentFrame);
    });
    
    MemCopyStage(nextFrame);
    HostDeviceCopyStage(nextFrame, nextFrameIntegrated);
    renderFuture.wait();
    displayFuture.wait();
}

void MemCopyBeacon::RenderStage(CrossFrameResource &ctx, D3D12_GPU_VIRTUAL_ADDRESS constantAddr)
{
    ctx.Reset3D();
    ctx.CmdList3D->RSSetViewports(1, &mViewPort);
    ctx.CmdList3D->RSSetScissorRects(1, &mScissor);
    {
        mGBufferPass->BeginPass(ctx.CmdList3D.Get());
        ctx.SetSceneConstant();
        mScene->RenderSphere(ctx.CmdList3D.Get(), constantAddr);
        mScene->RenderScene(ctx.CmdList3D.Get(), constantAddr);
        mGBufferPass->EndPass(ctx.CmdList3D.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    }
    {
        mLightPass->BeginPass(ctx.CmdList3D.Get(), D3D12_RESOURCE_STATE_COMMON);
        mScene->RenderQuad(ctx.CmdList3D.Get(), constantAddr);
        mLightPass->EndPass(ctx.CmdList3D.Get(), D3D12_RESOURCE_STATE_COMMON);
    }
    ctx.Signal3D(mDResource[Gpu::Discrete]->CmdQueue.Get());
    ctx.Sync3D();
}

void MemCopyBeacon::MemCopyStage(CrossFrameResource &ctx)
{
    ctx.mCopyFR->DeviceBufferToDeviceBuffer(mDResource[Gpu::Integrated]->Device.Get(),
                                            mDResource[Gpu::Integrated]->FR.at(0).GetResource("LightTexture")); // 动态构建Integrated GPU的资源
}

void MemCopyBeacon::DisplayStage(CrossFrameResource &ctx)
{
    ctx.Reset3D();
    ctx.CmdList3D->RSSetViewports(1, &mViewPort);
    ctx.CmdList3D->RSSetScissorRects(1, &mScissor);

    GResource::GPUTimer->BeginTimer(ctx.CmdList3D.Get(), 0);
    {
        mSobelPass->BeginPass(ctx.CmdList3D.Get());

        for (uint i = 0; i < GResource::config["Scene"]["PostProcessLoop"].as<uint>(); i++) {
            mSobelPass->ExecutePass(ctx.CmdList3D.Get());
        }

        mSobelPass->EndPass(ctx.CmdList3D.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    }
    {
        mQuadPass->BeginPass(ctx.CmdList3D.Get());
        ctx.CmdList3D->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        ctx.CmdList3D->IASetVertexBuffers(0, 1, &mIGpuQuadVBView);
        ctx.CmdList3D->DrawInstanced(4, 1, 0, 0);
        mQuadPass->EndPass(ctx.CmdList3D.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    // ===================== Integrated GPU 3D Stage =====================

    {
        GResource::GUIManager->DrawUI(ctx.CmdList3D.Get(), ctx.GetResource("SwapChain"), {});
    }

    ctx.Signal3D(mDResource[Gpu::Integrated]->CmdQueue.Get());
    mDResource[Gpu::Integrated]->SwapChain4->Present(0, 0);
    ctx.Sync3D();
}

void MemCopyBeacon::HostDeviceCopyStage(CrossFrameResource &resource, CrossFrameResource &ctx)
{
    ctx.ResetCopy();
    resource.mCopyFR->BufferToImage(mDResource[Gpu::Integrated]->Device.Get(),
                                    ctx.CopyCmdList.Get(),
                                    ctx.GetResource("LightTexture"));
    ctx.SignalCopy(mDResource[Gpu::Integrated]->CopyQueue.Get());
    ctx.SyncCopy(ctx.SharedFenceValue);
}
