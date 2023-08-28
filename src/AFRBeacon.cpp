#include "AFRBeacon.h"
#include "Framework/Application.h"
#include "GpuEntryLayout.h"

AFRBeacon::AFRBeacon(uint width, uint height, std::wstring title, uint frameCount) :
    RendererBase(width, height, title),
    mViewPort(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    FrameCount(frameCount),
    CurrentContextIndex(0)
{
}

AFRBeacon::~AFRBeacon()
{
}

void AFRBeacon::OnInit()
{
    auto handle = Application::GetHandle();
    CreateDeviceResource(handle);
    CompileShaders();
    CreateSignature2PSO();

    GResource::GUIManager->Init(mDisplayResource->Device.Get());

    CreateRtv(handle);

    CreateSharedFence();
    // Pass

    CreateQuad(); // Display GPU
    LoadAssets(); // Backend GPU

    InitSceneCB();
    InitPass();
}

void AFRBeacon::OnUpdate()
{
    auto [ctx, index] = GetCurrentContext();
    GResource::CPUTimerManager->BeginTimer("UpdateScene");
    auto [frameResource, frameIndex] = ctx->GetCurrentFrameResource();
    mScene->UpdateCamera();
    mScene->UpdateSceneConstant(frameResource->mSceneCB.SceneCB.get());
    GResource::CPUTimerManager->EndTimer("UpdateScene");
}

void AFRBeacon::OnRender()
{
    auto [ctx, index] = GetCurrentContext();
    GResource::CPUTimerManager->UpdateTimer("RenderTime");

    GResource::CPUTimerManager->BeginTimer("DrawCall");
    GResource::CPUTimerManager->BeginTimer("UpdatePass");
    SetPass(ctx, CurrentContextIndex == mBackendResource.size());
    GResource::CPUTimerManager->EndTimer("UpdatePass");
    GResource::CPUTimerManager->UpdateAvgTimer("AvgTime1000");

    TimePoint now = std::chrono::high_resolution_clock::now();
    static uint fpsCount = 0;
    fpsCount++;
    auto duration = GResource::CPUTimerManager->GetStaticTimeDuration("FPS", now);
    if (duration > 1000) {
        GResource::CPUTimerManager->SetStaticTime("FPS", now);
        GResource::GUIManager->State.FPSCount = fpsCount;
        fpsCount = 0;
    }

    SyncExecutePass(ctx, index == mBackendResource.size());

    GResource::CPUTimerManager->EndTimer("DrawCall");

    ctx->IncreaseFrameIndex();
    IncrementContextIndex();
}

void AFRBeacon::OnKeyDown(byte key)
{}

void AFRBeacon::OnMouseDown(WPARAM btnState, int x, int y)
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

void AFRBeacon::OnDestory()
{
    for (auto &backend : mBackendResource) {
        for (auto &frameResource : backend->mSFR) {
            frameResource.FlushCopy();
            frameResource.FlushDirect();
        }
    }
    for (auto &backendResource : mDisplayResource->mSharedFR) {
        for (auto &frameResource : backendResource) {
            frameResource.SignalDirect(mDisplayResource->DirectQueue.Get());
            frameResource.FlushDirect();
        }
    }
    for (auto &displayResource : mDisplayResource->mSFR) {
        displayResource.SignalDirect(mDisplayResource->DirectQueue.Get());
        displayResource.FlushDirect();
    }
    mScene = nullptr;
    mBackendResource.clear();
    mDisplayResource = nullptr;
    mFactory = nullptr;
}

void AFRBeacon::CreateDeviceResource(HWND handle)
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
        // if (output != nullptr) continue;
        if (SUCCEEDED(hr) && output != nullptr) {
            auto outputStr = std::format(L"iGPU|tIndex: {} DeviceName: {}", i, str);
            OutputDebugStringW(outputStr.c_str());
            mDisplayResource = std::make_unique<AFRDisplayResource>(mFactory.Get(), adapter.Get(), FrameCount, outputStr);
            break;
        } else {
            static uint id = 0;
            auto outputStr = std::format(L"dGPU|Index: {} DeviceName: {}", i, str);
            OutputDebugStringW(outputStr.c_str());
            auto backendResource = std::make_unique<AFRBackendResource>(mFactory.Get(), adapter.Get(), FrameCount, outputStr);
            mBackendResource.push_back(std::move(backendResource));
        }
    }
    // if (mBackendResource.empty()) {
    //     throw std::runtime_error("No backend device found");
    // }
}

void AFRBeacon::CompileShaders()
{
    std::array<std::wstring, 5> shaderPaths = {
        L"Shaders/GBuffer.hlsl",
        L"Shaders/LightingPass.hlsl",
        L"Shaders/PostProcess.hlsl",
        L"Shaders/ScreenQuad.hlsl",
        L"Shaders/QuadTest.hlsl"};

    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    ComPtr<ID3DBlob> errorInfo;

    ThrowIfFailed(D3DCompileFromFile(shaderPaths[4].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["SingleQuadVS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[4].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["SingleQuadPS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[3].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["ScreenQuadVS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[3].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["ScreenQuadPS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[2].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "SobelMain", "cs_5_1", compileFlags, 0, &GResource::Shaders["SobelCS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[0].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["GBufferVS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[0].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["GBufferPS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[1].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["LightPassVS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[1].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["LightPassPS"], &errorInfo));

    if (errorInfo != nullptr) {
        OutputDebugStringA(static_cast<char *>(errorInfo->GetBufferPointer()));
    }
}
void AFRBeacon::CreateSignature2PSO()
{
    GResource::InputLayout = GpuEntryLayout::CreateInputLayout();
    for (const auto &backendResource : mBackendResource) {
        auto *device = backendResource->Device.Get();
        backendResource->Signature["Graphics"] = GpuEntryLayout::CreateRenderSignature(device, GBufferPass::GetTargetCount());
        backendResource->PSO["GBuffer"] = GpuEntryLayout::CreateGBufferPassPSO(device, backendResource->Signature["Graphics"].Get(), GBufferPass::GetTargetFormat(), GBufferPass::GetDepthFormat());
        backendResource->PSO["LightPass"] = GpuEntryLayout::CreateLightPassPSO(device, backendResource->Signature["Graphics"].Get());

        backendResource->Signature["Graphics"] = GpuEntryLayout::CreateRenderSignature(device, GBufferPass::GetTargetCount());
        backendResource->PSO["MiaxQuadPass"] = GpuEntryLayout::CreateMixQuadPassPSO(device, backendResource->Signature["Graphics"].Get());

        backendResource->PSO["SingleQuadPass"] = GpuEntryLayout::CreateSingleQuadPassPSO(device, backendResource->Signature["Graphics"].Get());

        backendResource->Signature["Compute"] = GpuEntryLayout::CreateComputeSignature(device);
        backendResource->PSO["Sobel"] = GpuEntryLayout::CreateSobelPSO(device, backendResource->Signature["Compute"].Get());
    }

    auto *device = mDisplayResource->Device.Get();

    mDisplayResource->Signature["Graphics"] = GpuEntryLayout::CreateRenderSignature(device, GBufferPass::GetTargetCount());
    mDisplayResource->PSO["GBuffer"] = GpuEntryLayout::CreateGBufferPassPSO(device, mDisplayResource->Signature["Graphics"].Get(), GBufferPass::GetTargetFormat(), GBufferPass::GetDepthFormat());
    mDisplayResource->PSO["LightPass"] = GpuEntryLayout::CreateLightPassPSO(device, mDisplayResource->Signature["Graphics"].Get());

    mDisplayResource->Signature["Graphics"] = GpuEntryLayout::CreateRenderSignature(device, GBufferPass::GetTargetCount());
    mDisplayResource->PSO["MixQuadPass"] = GpuEntryLayout::CreateMixQuadPassPSO(device, mDisplayResource->Signature["Graphics"].Get());

    mDisplayResource->Signature["Compute"] = GpuEntryLayout::CreateComputeSignature(device);
    mDisplayResource->PSO["Sobel"] = GpuEntryLayout::CreateSobelPSO(device, mDisplayResource->Signature["Compute"].Get());

    mDisplayResource->PSO["SingleQuadPass"] = GpuEntryLayout::CreateSingleQuadPassPSO(device, mDisplayResource->Signature["Graphics"].Get());
}

bool AFRBeacon::CheckFeatureSupport()
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS stOptions = {};
    // 通过检测带有显示输出的显卡是否支持跨显卡资源来决定跨显卡的资源如何创建

    ThrowIfFailed(mDisplayResource->Device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS, reinterpret_cast<void *>(&stOptions), sizeof(stOptions)));
    auto crossAdaptorTextureSupport = stOptions.CrossAdapterRowMajorTextureSupported;
    if (!crossAdaptorTextureSupport) {
        OutputDebugStringA("Displat Device Cross adaptor texture not supported\n");
        return false;
    }
    for (auto &backend : mBackendResource) {
        ThrowIfFailed(backend->Device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS, reinterpret_cast<void *>(&stOptions), sizeof(stOptions)));
        crossAdaptorTextureSupport = stOptions.CrossAdapterRowMajorTextureSupported;
        if (!crossAdaptorTextureSupport) {
            OutputDebugStringA("Backend Device Cross adaptor texture not supported\n");
            return false;
        }
    }
    return true;
}

void AFRBeacon::CreateRtv(HWND handle)
{
    // Display SwapChain
    mDisplayResource->CreateSwapChain(mFactory.Get(), handle, GetWidth(), GetHeight(), mBackendResource.size());

    {
        CrossAdapterTextureSupport = false;

        auto handles = mDisplayResource->CreateRenderTargets(GetWidth(), GetHeight(), mBackendResource.size());
        for (auto i = 0; i < mBackendResource.size(); i++) {
            mBackendResource[i]->CreateRenderTargetByHandle(GetWidth(), GetHeight(), handles[i]);
        }
    }
}

void AFRBeacon::CreateSharedFence()
{
    auto handles = mDisplayResource->CreateSharedFence(mBackendResource.size());
    for (size_t i = 0; i < mBackendResource.size(); i++) {
        std::vector<HANDLE> frameHandles(handles.begin() + i * FrameCount, handles.begin() + (i + 1) * FrameCount);
        mBackendResource[i]->CreateSharedFenceByHandles(frameHandles);
    }
}

void AFRBeacon::LoadAssets()
{
    // Backend Device VB IB
    std::string assetsPath = GResource::config["Scene"]["ScenePath"].as<std::string>();
    auto sceneName = GResource::config["Scene"]["SceneName"].as<std::string>();
    auto scene = std::make_unique<Scene>(assetsPath, sceneName);
    for (const auto &backend : mBackendResource) {
        auto *device = backend->Device.Get();
        auto &frameResource = backend->mSFR.at(0);
        frameResource.ResetDirect();
        // VB IB
        scene->InitWithBackend(device,
                               frameResource.DirectCmdList.Get(),
                               backend->mResourceRegister->SrvCbvUavDescriptorHeap.get(),
                               backend->mSceneVB,
                               backend->mSceneIB,
                               backend->mSceneTextures);
        backend->mEnvMapIndex = scene->GetEnvSrvIndex();
        // Frame CB
        for (auto &fr : backend->mSFR) {
            fr.SetSceneTextureBase(0); // DiffuseMap取的是所有SRV中的ID因而Texture的基准应该从第0个开始
            fr.CreateConstantBuffer(device,
                                    scene->GetEntityCount(),
                                    1,
                                    scene->GetMaterialCount());
        }
        backend->mRenderItems = scene->GetRenderItems();
        frameResource.SubmitDirect(backend->DirectQueue.Get());
        frameResource.SignalDirect(backend->DirectQueue.Get());
        frameResource.FlushDirect();
    }

    // Display Resource
    {
        auto *device = mDisplayResource->Device.Get();

        auto &frameResource = mDisplayResource->mSFR.at(0);
        frameResource.ResetDirect();
        // VB IB
        scene->InitWithBackend(device,
                               frameResource.DirectCmdList.Get(),
                               mDisplayResource->mResourceRegister->SrvCbvUavDescriptorHeap.get(),
                               mDisplayResource->mSceneVB,
                               mDisplayResource->mSceneIB,
                               mDisplayResource->mSceneTextures);
        mDisplayResource->mEnvMapIndex = scene->GetEnvSrvIndex();
        // Frame CB
        for (auto &fr : mDisplayResource->mSFR) {
            fr.SetSceneTextureBase(0); // DiffuseMap取的是所有SRV中的ID因而Texture的基准应该从第0个开始
            fr.CreateConstantBuffer(device,
                                    scene->GetEntityCount(),
                                    1,
                                    scene->GetMaterialCount());
        }

        mDisplayResource->mRenderItems = scene->GetRenderItems();
        frameResource.SubmitDirect(mDisplayResource->DirectQueue.Get());
        frameResource.SignalDirect(mDisplayResource->DirectQueue.Get());
        frameResource.FlushDirect();
    }
    mScene = std::move(scene);
}

void AFRBeacon::CreateQuad()
{
    auto *device = mDisplayResource->Device.Get();
    auto &frameResource = mDisplayResource->mSFR.at(0);
    mScene = std::make_unique<Scene>("Assets", "lighthouse");
    frameResource.ResetDirect();
    mScene->InitWithDisplay(device,
                            frameResource.DirectCmdList.Get(),
                            mDisplayResource->mQuadVB,
                            mDisplayResource->mQuadIB);
    mDisplayResource->CreateScreenQuadView();
    frameResource.SubmitDirect(mDisplayResource->DirectQueue.Get());
    frameResource.SignalDirect(mDisplayResource->DirectQueue.Get());
    frameResource.FlushDirect();
}
void AFRBeacon::InitSceneCB()
{
    // Backend Device Local FrameBuffer
    for (const auto &backendResource : mBackendResource) {
        // Frame CB
        for (auto &fr : backendResource->mSFR) {
            mScene->UpdateSceneConstant(fr.mSceneCB.SceneCB.get());
            mScene->UpdateEntityConstant(fr.mSceneCB.EntityCB.get());
            mScene->UpdateLightConstant(fr.mSceneCB.LightCB.get());
            mScene->UpdateMaterialConstant(fr.mSceneCB.MaterialCB.get());
        }
    }
    // Display Resource
    for (const auto &fr : mDisplayResource->mSFR) {
        mScene->UpdateSceneConstant(fr.mSceneCB.SceneCB.get());
        mScene->UpdateEntityConstant(fr.mSceneCB.EntityCB.get());
        mScene->UpdateLightConstant(fr.mSceneCB.LightCB.get());
        mScene->UpdateMaterialConstant(fr.mSceneCB.MaterialCB.get());
    }
}

std::tuple<AFRResourceBase *, uint> AFRBeacon::GetCurrentContext() const
{
    if (CurrentContextIndex < mBackendResource.size()) {
        return {mBackendResource[CurrentContextIndex].get(), CurrentContextIndex};
    } else {
        return {mDisplayResource.get(), CurrentContextIndex};
    }
}

void AFRBeacon::IncrementContextIndex()
{
    CurrentContextIndex = (CurrentContextIndex + 1) % (mBackendResource.size() + 1);
}

void AFRBeacon::InitPass()
{
    mDisplayResource->mSobelPass = std::make_unique<SobelPass>(
        mDisplayResource->PSO["Sobel"].Get(),
        mDisplayResource->Signature["Compute"].Get());
    mDisplayResource->mMixPass = std::make_unique<MixQuadPass>(
        mDisplayResource->PSO["MixQuadPass"].Get(),
        mDisplayResource->Signature["Graphics"].Get());
    mDisplayResource->mGBufferPass = std::make_unique<GBufferPass>(
        mDisplayResource->PSO["GBuffer"].Get(),
        mDisplayResource->Signature["Graphics"].Get());
    mDisplayResource->mLightPass = std::make_unique<LightPass>(
        mDisplayResource->PSO["LightPass"].Get(),
        mDisplayResource->Signature["Graphics"].Get());
    mDisplayResource->mSingleQuadPass = std::make_unique<SingleQuadPass>(
        mDisplayResource->PSO["SingleQuadPass"].Get(),
        mDisplayResource->Signature["Graphics"].Get());
    for (auto &backend : mBackendResource) {
        backend->mGBufferPass = std::make_unique<GBufferPass>(
            backend->PSO["GBuffer"].Get(),
            backend->Signature["Graphics"].Get());
        backend->mLightPass = std::make_unique<LightPass>(
            backend->PSO["LightPass"].Get(),
            backend->Signature["Graphics"].Get());
        backend->mSobelPass = std::make_unique<SobelPass>(
            backend->PSO["Sobel"].Get(),
            backend->Signature["Compute"].Get());
        backend->mMixPass = std::make_unique<MixQuadPass>(
            backend->PSO["MixQuadPass"].Get(),
            backend->Signature["Graphics"].Get());
    }
}

void AFRBeacon::SetPass(AFRResourceBase *ctx, bool isDisplay)
{
    auto [frameResource, frameIndex] = ctx->GetCurrentFrameResource();

    // GBuffer Pass
    auto &gbufferPass = *(ctx->mGBufferPass);
    std::vector<ID3D12Resource *> gbufferTargets;
    for (uint i = 0; i < GBufferPass::GetTargetCount(); i++) {
        auto name = std::format("GBuffer{}", i);
        gbufferTargets.push_back(frameResource->GetResource(name));
    }
    auto rtvHandle = frameResource->GetRtv("GBuffer0");
    auto dsvHandle = frameResource->GetDsv("Depth");

    gbufferPass.SetRenderTarget(gbufferTargets, rtvHandle);
    gbufferPass.SetDepthBuffer(frameResource->GetResource("Depth"), dsvHandle);
    gbufferPass.SetRTVDescriptorSize(ctx->mRTVDescriptorSize);

    // Light Pass
    auto &lightPass = ctx->mLightPass;
    lightPass->SetRenderTarget(frameResource->GetResource("Light"), frameResource->GetRtv("Light"));
    lightPass->SetGBuffer(frameResource->mSrvCbvUavHeap->Resource(), frameResource->GetSrvCbvUav("GBuffer0"));
    lightPass->SetTexture(frameResource->mSrvCbvUavHeap->Resource(), frameResource->GetSrvCbvUav("SceneTextureBase"));
    lightPass->SetCubeMap(ctx->mResourceRegister->SrvCbvUavDescriptorHeap->Resource(),
                          ctx->mResourceRegister->SrvCbvUavDescriptorHeap->GPUHandle(ctx->mEnvMapIndex));

    // Sobel Pass
    auto &sobelPass = ctx->mSobelPass;
    sobelPass->SetInput(frameResource->GetSrvCbvUav("Light"));
    sobelPass->SetSrvHeap(frameResource->mSrvCbvUavHeap->Resource());
    sobelPass->SetTarget(frameResource->GetResource("Sobel"), frameResource->GetSrvCbvUav("SobelUVA"));

    // Mix Quad Pass
    auto &mixQuadPass = ctx->mMixPass;
    mixQuadPass->SetTarget(frameResource->GetResource("MixTex"), frameResource->GetRtv("MixTex"));
    mixQuadPass->SetSrvHandle(frameResource->GetSrvCbvUav("Light")); //  LightCopy SobelSRV Sobel UVA SRV Sobel SwapChain
    mixQuadPass->SetRenderType(QuadShader::MixQuad);

    // Quad Pass
    if (isDisplay) {
        auto &quadPass = mDisplayResource->mSingleQuadPass;
        auto [frame, index] = mDisplayResource->GetCurrentFrameResource();
        auto t1 = frame->GetResource("SwapChain");
        auto t2 = frame->GetRtv("SwapChain");
        quadPass->SetTarget(frame->GetResource("SwapChain"), frame->GetRtv("SwapChain"));
        quadPass->SetSrvHandle(frame->GetSrvCbvUav("MixTex"));
    } else {
        auto &quadPass = mDisplayResource->mMixPass;
        auto *frame = mDisplayResource->GetSharedFrameResource(CurrentContextIndex, frameIndex);
        quadPass->SetTarget(frame->GetResource("SwapChain"), frameResource->GetRtv("SwapChain"));
        quadPass->SetSrvHandle(frame->GetSrvCbvUav("LightCopy"));
    }
}

void AFRBeacon::SyncExecutePass(AFRResourceBase *ctx, bool isDisplay)
{
    auto [frame, frameIndex] = ctx->GetCurrentFrameResource();

    // =============================== Stage 1 DeferredRendering ===============================
    frame->FlushDirect();
    frame->ResetDirect();
    frame->DirectCmdList->RSSetViewports(1, &mViewPort);
    frame->DirectCmdList->RSSetScissorRects(1, &mScissor);

    auto entitiesCB = frame->mSceneCB.EntityCB->resource()->GetGPUVirtualAddress();
    auto &gbufferPass = *(ctx->mGBufferPass);
    {
        gbufferPass.BeginPass(frame->DirectCmdList.Get());
        frame->SetCurrentFrameCB();
        mScene->RenderScene(frame->DirectCmdList.Get(), entitiesCB, &ctx->mRenderItems);
        mScene->RenderSphere(frame->DirectCmdList.Get(), entitiesCB, &ctx->mRenderItems);
        gbufferPass.EndPass(frame->DirectCmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    auto &lightPass = *(ctx->mLightPass);
    {
        lightPass.BeginPass(frame->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON);
        mScene->RenderQuad(frame->DirectCmdList.Get(), entitiesCB, &ctx->mRenderItems);
        lightPass.EndPass(frame->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON);
    }

    auto &sobelPass = *(ctx->mSobelPass);
    {
        sobelPass.BeginPass(frame->DirectCmdList.Get());
        sobelPass.ExecutePass(frame->DirectCmdList.Get());
        sobelPass.EndPass(frame->DirectCmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    auto &mixPass = *(ctx->mMixPass);
    {
        mixPass.BeginPass(frame->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON, false);
        mScene->RenderScreenQuad(frame->DirectCmdList.Get(), &mDisplayResource->ScreenQuadVBView, &mDisplayResource->ScreenQuadIBView);
        mixPass.EndPass(frame->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON);
    }

    StageFrameResource *displayFrame = nullptr;
    if (!isDisplay) {
        displayFrame->FlushDirect();
        displayFrame->ResetDirect();
        displayFrame->DirectCmdList->RSSetViewports(1, &mViewPort);
        displayFrame->DirectCmdList->RSSetScissorRects(1, &mScissor);
        // TODO Copy Render Result
        // ========================Stage 2 Copy Texture ========================
        // stage2->FlushCopy(); // 等待上一帧拷贝完毕
        // stage2->ResetCopy();
        // ctx->CopyQueue->Wait(stage2->Fence.Get(), stage2->FenceValue); // 等待上一帧渲染完毕

        // {
        //     if (CrossAdapterTextureSupport) {
        //         stage2->CopyCmdList->CopyResource(stage2->GetResource("LightCopy"), stage2->GetResource("Light"));
        //     } else {
        //         auto copyDstBarrier = CD3DX12_RESOURCE_BARRIER::Transition(stage2->GetResource("LightCopyBuffer"), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        //         auto copySrcBarrier = CD3DX12_RESOURCE_BARRIER::Transition(stage2->GetResource("LightCopyBuffer"), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);

        //         auto targetDesc = stage2->GetResource("Light")->GetDesc();
        //         D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};

        //         ctx->Device->GetCopyableFootprints(&targetDesc, 0, 1, 0, &layout, nullptr, nullptr, nullptr);

        //         CD3DX12_TEXTURE_COPY_LOCATION dst(stage2->GetResource("LightCopyBuffer"), layout);
        //         CD3DX12_TEXTURE_COPY_LOCATION src(stage2->GetResource("Light"), 0);
        //         // CD3DX12_BOX box(0, 0, GetWidth(), GetHeight());

        //         stage2->CopyCmdList->ResourceBarrier(1, &copyDstBarrier);
        //         stage2->CopyCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        //         stage2->CopyCmdList->ResourceBarrier(1, &copySrcBarrier);
        //     }
        // }
        // stage2->SubmitCopy(backend->CopyQueue.Get());
        // stage2->SignalCopy(backend->CopyQueue.Get());

        // // ========================Stage 3 PostProcess ========================
        // stage3->FlushDirect();
        // stage3->ResetDirect();
        // mDisplayResource->DirectQueue->Wait(stage3->SharedFence.Get(), stage3Backend->SharedFenceValue);
        // stage3->DirectCmdList->RSSetViewports(1, &mViewPort);
        // stage3->DirectCmdList->RSSetScissorRects(1, &mScissor);

        // // Copy Light
        // {
        //     auto copyDstBarrier = CD3DX12_RESOURCE_BARRIER::Transition(stage3->GetResource("LightCopy"), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        //     auto shaderResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(stage3->GetResource("LightCopy"), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);

        //     auto targetDesc = stage3->GetResource("LightCopy")->GetDesc();
        //     D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
        //     mDisplayResource->Device->GetCopyableFootprints(&targetDesc, 0, 1, 0, &layout, nullptr, nullptr, nullptr);

        //     CD3DX12_TEXTURE_COPY_LOCATION dst(stage3->GetResource("LightCopy"), 0);
        //     CD3DX12_TEXTURE_COPY_LOCATION src(stage3->GetResource("LightCopyBuffer"), layout);
        //     CD3DX12_BOX box(0, 0, GetWidth(), GetHeight());

        //     stage3->DirectCmdList->ResourceBarrier(1, &copyDstBarrier);
        //     stage3->DirectCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, &box);
        //     stage3->DirectCmdList->ResourceBarrier(1, &shaderResourceBarrier);
        // }

        // }
    } else {
        auto target = mDisplayResource->GetCurrentFrameResource();
        displayFrame = std::get<0>(target);
    }

    // =============================== Stage 3 Display ===============================

    auto &quadPass = mDisplayResource->mSingleQuadPass;
    {
        quadPass->BeginPass(displayFrame->DirectCmdList.Get());
        mScene->RenderScreenQuad(displayFrame->DirectCmdList.Get(),
                                 &mDisplayResource->ScreenQuadVBView,
                                 &mDisplayResource->ScreenQuadIBView);
        quadPass->EndPass(displayFrame->DirectCmdList.Get(), D3D12_RESOURCE_STATE_PRESENT);
    }
    // GResource::GUIManager->DrawUI(displayFrame->DirectCmdList.Get(), displayFrame->GetResource("SwapChain"));

    displayFrame->SubmitDirect(mDisplayResource->DirectQueue.Get());
    displayFrame->SignalDirect(mDisplayResource->DirectQueue.Get());

    mDisplayResource->SwapChain->Present(0, 0);
}
