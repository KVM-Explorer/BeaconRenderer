#include "StageBeacon.h"
#include "Framework/Application.h"
#include "GpuEntryLayout.h"

StageBeacon::StageBeacon(uint width, uint height, std::wstring title, uint frameCount) :
    RendererBase(width, height, title),
    mViewPort(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    FrameCount(frameCount),
    CurrentBackendIndex(0),
    mStagePipeline(9, 3)
{
}

StageBeacon::~StageBeacon()
{
}

void StageBeacon::OnInit()
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
    InitPipelineFunction();
}

void StageBeacon::OnUpdate()
{
    auto [backend, deviceIndex] = GetCurrentBackend();
    auto [frameResource, frameIndex] = backend->GetCurrentFrame(Stage::DeferredRendering);
    mScene->UpdateCamera();
    mScene->UpdateSceneConstant(frameResource->mSceneCB.SceneCB.get());
}

void StageBeacon::OnRender()
{
    // if (!GResource::GUIManager->State.DrawCallAsync) {
    //     GResource::CPUTimerManager->UpdateTimer("RenderTime");
    //     GResource::CPUTimerManager->BeginTimer("DrawCall");
    //     SyncDrawCall();
    //     GResource::CPUTimerManager->EndTimer("DrawCall");
    // } 
    // else {
        AsyncDrawCall();
    // }
}

void StageBeacon::OnKeyDown(byte key)
{}

void StageBeacon::OnMouseDown(WPARAM btnState, int x, int y)
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

void StageBeacon::OnDestory()
{
    mStagePipeline.Stop();

    for (auto &backend : mBackendResource) {
        for (auto &frameResource : backend->mSFR) {
            frameResource.FlushCopy();
            frameResource.FlushDirect();
        }
    }
    for (auto &backendResource : mDisplayResource->mSFR) {
        for (auto &frameResource : backendResource) {
            frameResource.SignalDirect(mDisplayResource->DirectQueue.Get());
            frameResource.FlushDirect();
        }
    }
    mScene = nullptr;
    mBackendResource.clear();
    mDisplayResource = nullptr;
    mFactory = nullptr;
}

void StageBeacon::CreateDeviceResource(HWND handle)
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
            mDisplayResource = std::make_unique<DisplayResource>(mFactory.Get(), adapter.Get(), FrameCount);
        } else {
            static uint id = 0;
            auto outputStr = std::format(L"dGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
            OutputDebugStringW(outputStr.c_str());
            // auto startFrameIndex = GetBackendStartFrameIndex(mBackendResource.size()); TODO : 自适应调整
            auto startFrameIndex = id;
            auto backendResource = std::make_unique<BackendResource>(mFactory.Get(), adapter.Get(), FrameCount, startFrameIndex);
            mBackendResource.push_back(std::move(backendResource));
            id++;
        }
    }
    if (mBackendResource.empty()) {
        throw std::runtime_error("No backend device found");
    }
}

void StageBeacon::CompileShaders()
{
    std::array<std::wstring, 4> shaderPaths = {
        L"Shaders/GBuffer.hlsl",
        L"Shaders/LightingPass.hlsl",
        L"Shaders/PostProcess.hlsl",
        L"Shaders/ScreenQuad.hlsl",
    };

    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    ComPtr<ID3DBlob> errorInfo;
    // Display GPU
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[3].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["ScreenQuadVS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[3].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["ScreenQuadPS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[2].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "SobelMain", "cs_5_1", compileFlags, 0, &GResource::Shaders["SobelCS"], &errorInfo));

    // Backend GPU
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[0].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["GBufferVS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[0].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["GBufferPS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[1].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["LightPassVS"], &errorInfo));
    ThrowIfFailed(D3DCompileFromFile(shaderPaths[1].c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["LightPassPS"], &errorInfo));
    if (errorInfo != nullptr) {
        OutputDebugStringA(static_cast<char *>(errorInfo->GetBufferPointer()));
    }
}
void StageBeacon::CreateSignature2PSO()
{
    GResource::InputLayout = GpuEntryLayout::CreateInputLayout();
    for (const auto &backendResource : mBackendResource) {
        auto *device = backendResource->Device.Get();
        backendResource->Signature["Graphics"] = GpuEntryLayout::CreateRenderSignature(device, GBufferPass::GetTargetCount());
        backendResource->PSO["GBuffer"] = GpuEntryLayout::CreateGBufferPassPSO(device, backendResource->Signature["Graphics"].Get(), GBufferPass::GetTargetFormat(), GBufferPass::GetDepthFormat());
        backendResource->PSO["LightPass"] = GpuEntryLayout::CreateLightPassPSO(device, backendResource->Signature["Graphics"].Get());
    }

    auto *device = mDisplayResource->Device.Get();
    // TODO
    mDisplayResource->Signature["Graphics"] = GpuEntryLayout::CreateRenderSignature(device, GBufferPass::GetTargetCount());
    mDisplayResource->PSO["QuadPass"] = GpuEntryLayout::CreateQuadPassPSO(device, mDisplayResource->Signature["Graphics"].Get());

    mDisplayResource->Signature["Compute"] = GpuEntryLayout::CreateComputeSignature(device);
    mDisplayResource->PSO["Sobel"] = GpuEntryLayout::CreateSobelPSO(device, mDisplayResource->Signature["Compute"].Get());
}

void StageBeacon::CreateRtv(HWND handle)
{
    // Display SwapChain
    mDisplayResource->CreateSwapChain(mFactory.Get(), handle, GetWidth(), GetHeight(), mBackendResource.size());

    // Display Device Local FrameBuffer
    auto handles = mDisplayResource->CreateRenderTargets(GetWidth(), GetHeight(), mBackendResource.size());

    // Backend Device Local FrameBuffer
    for (size_t i = 0; i < mBackendResource.size(); i++) {
        std::vector<HANDLE> frameHandles(handles.begin() + i * FrameCount, handles.begin() + (i + 1) * FrameCount);
        mBackendResource[i]->CreateRenderTargets(GetWidth(), GetHeight(), frameHandles);
    }
}

void StageBeacon::CreateSharedFence()
{
    auto handles = mDisplayResource->CreateSharedFence(mBackendResource.size());
    for (size_t i = 0; i < mBackendResource.size(); i++) {
        std::vector<HANDLE> frameHandles(handles.begin() + i * FrameCount, handles.begin() + (i + 1) * FrameCount);
        mBackendResource[i]->CreateSharedFence(frameHandles);
    }
}

void StageBeacon::LoadAssets()
{
    // Backend Device VB IB
    std::string assetsPath = "Assets";
    auto scene = std::make_unique<Scene>(assetsPath, "witch");
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
    mScene = std::move(scene);
}

void StageBeacon::CreateQuad()
{
    auto *device = mDisplayResource->Device.Get();
    auto &frameResource = mDisplayResource->mSFR.at(0).at(0);
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
void StageBeacon::InitSceneCB()
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
}

std::tuple<BackendResource *, uint> StageBeacon::GetCurrentBackend() const
{
    return {mBackendResource[CurrentBackendIndex].get(), CurrentBackendIndex};
}

void StageBeacon::IncrementBackendIndex()
{
    CurrentBackendIndex = (CurrentBackendIndex + 1) % mBackendResource.size();
}

uint StageBeacon::GetBackendStartFrameIndex(uint index) const
{
    const uint stageCount = 3;
    return (index + stageCount - 1) % FrameCount;
}

void StageBeacon::InitPass()
{
    mDisplayResource->mSobelPass = std::make_unique<SobelPass>(
        mDisplayResource->PSO["Sobel"].Get(),
        mDisplayResource->Signature["Compute"].Get());
    mDisplayResource->mQuadPass = std::make_unique<QuadPass>(
        mDisplayResource->PSO["QuadPass"].Get(),
        mDisplayResource->Signature["Graphics"].Get());
    for (auto &backend : mBackendResource) {
        backend->mGBufferPass = std::make_unique<GBufferPass>(
            backend->PSO["GBuffer"].Get(),
            backend->Signature["Graphics"].Get());
        backend->mLightPass = std::make_unique<LightPass>(
            backend->PSO["LightPass"].Get(),
            backend->Signature["Graphics"].Get());
    }
}

void StageBeacon::SyncDrawCall()
{
    auto [backend, deviceIndex] = GetCurrentBackend();
    SetMultiStagePass(backend, deviceIndex);
    SyncExecutePass(backend, deviceIndex);
    IncrementBackendIndex();
}

void StageBeacon::SyncSingleDrawCall()
{
    auto [backend, deviceIndex] = GetCurrentBackend();
    SetMultiStagePass(backend, deviceIndex);
    SyncSingleExecutePass(backend, deviceIndex);
}

void StageBeacon::AsyncDrawCall()
{
    auto [backend, deviceIndex] = GetCurrentBackend();
    SetFrameStagePass(backend, deviceIndex);
    AsyncExecutePass(backend, deviceIndex);
}

void StageBeacon::SetFrameStagePass(BackendResource *backend, uint backendIndex)
{
    auto [stage, frameIndex] = backend->GetCurrentFrame(Stage::DeferredRendering);
    auto [displayStage, displayFrameIndex] = mDisplayResource->GetCurrentSingleFrame(backendIndex, Stage::PostProcess, frameIndex);

    auto &gbufferPass = *(backend->mGBufferPass);
    std::vector<ID3D12Resource *> gbufferTargets;
    for (uint i = 0; i < GBufferPass::GetTargetCount(); i++) {
        auto name = std::format("GBuffer{}", i);
        gbufferTargets.push_back(stage->GetResource(name));
    }
    auto rtvHandle = stage->GetRtv("GBuffer0");
    auto dsvHandle = stage->GetDsv("Depth");

    gbufferPass.SetRenderTarget(gbufferTargets, rtvHandle);
    gbufferPass.SetDepthBuffer(stage->GetResource("Depth"), dsvHandle);
    gbufferPass.SetRTVDescriptorSize(backend->mRTVDescriptorSize);

    // Light Pass
    auto &lightPass = backend->mLightPass;
    lightPass->SetRenderTarget(stage->GetResource("Light"), stage->GetRtv("Light"));
    lightPass->SetGBuffer(stage->mSrvCbvUavHeap->Resource(), stage->GetSrvCbvUav("GBuffer0"));
    lightPass->SetTexture(stage->mSrvCbvUavHeap->Resource(), stage->GetSrvCbvUav("SceneTextureBase"));

    auto &sobelPass = mDisplayResource->mSobelPass;

    sobelPass->SetInput(displayStage->GetSrvCbvUav("LightCopy"));
    sobelPass->SetSrvHeap(displayStage->mSrvCbvUavHeap->Resource());
    sobelPass->SetTarget(displayStage->GetResource("Sobel"), displayStage->GetSrvCbvUav("SobelUVA"));

    // Quad Pass
    auto &quadPass = mDisplayResource->mQuadPass;
    quadPass->SetTarget(displayStage->GetResource("SwapChain"), displayStage->GetRtv("SwapChain"));
    quadPass->SetSrvHandle(displayStage->GetSrvCbvUav("LightCopy")); //  LightCopy SobelSRV Sobel UVA SRV Sobel SwapChain
    quadPass->SetRenderType(QuadShader::MixQuad);
}

void StageBeacon::SetMultiStagePass(BackendResource *backend, uint backendIndex)
{
    auto [stage1Resource, frameIndex] = backend->GetCurrentFrame(Stage::DeferredRendering);

    // GBuffer Pass
    auto &gbufferPass = *(backend->mGBufferPass);
    std::vector<ID3D12Resource *> gbufferTargets;
    for (uint i = 0; i < GBufferPass::GetTargetCount(); i++) {
        auto name = std::format("GBuffer{}", i);
        gbufferTargets.push_back(stage1Resource->GetResource(name));
    }
    auto rtvHandle = stage1Resource->GetRtv("GBuffer0");
    auto dsvHandle = stage1Resource->GetDsv("Depth");

    gbufferPass.SetRenderTarget(gbufferTargets, rtvHandle);
    gbufferPass.SetDepthBuffer(stage1Resource->GetResource("Depth"), dsvHandle);
    gbufferPass.SetRTVDescriptorSize(backend->mRTVDescriptorSize);

    // Light Pass
    auto &lightPass = backend->mLightPass;
    lightPass->SetRenderTarget(stage1Resource->GetResource("Light"), stage1Resource->GetRtv("Light"));
    lightPass->SetGBuffer(stage1Resource->mSrvCbvUavHeap->Resource(), stage1Resource->GetSrvCbvUav("GBuffer0"));
    lightPass->SetTexture(stage1Resource->mSrvCbvUavHeap->Resource(), stage1Resource->GetSrvCbvUav("SceneTextureBase"));

    // Sobel Pass
    auto [stage3Resource, index] = mDisplayResource->GetCurrentFrame(backendIndex, Stage::PostProcess, frameIndex);
    auto &sobelPass = mDisplayResource->mSobelPass;
    sobelPass->SetInput(stage3Resource->GetSrvCbvUav("LightCopy"));
    sobelPass->SetSrvHeap(stage3Resource->mSrvCbvUavHeap->Resource());
    sobelPass->SetTarget(stage3Resource->GetResource("Sobel"), stage3Resource->GetSrvCbvUav("SobelUVA"));

    // Quad Pass
    auto &quadPass = mDisplayResource->mQuadPass;
    quadPass->SetTarget(stage3Resource->GetResource("SwapChain"), stage3Resource->GetRtv("SwapChain"));
    quadPass->SetSrvHandle(stage3Resource->GetSrvCbvUav("LightCopy")); //  LightCopy SobelSRV Sobel UVA SRV Sobel SwapChain
    quadPass->SetRenderType(QuadShader::MixQuad);
}

void StageBeacon::SyncExecutePass(BackendResource *backend, uint backendIndex)
{
    auto [stage1, stage1Index] = backend->GetCurrentFrame(Stage::DeferredRendering);
    auto [stage2, stage2Index] = backend->GetCurrentFrame(Stage::CopyTexture);
    auto [stage3Backend, stage3BackendIndex] = backend->GetCurrentFrame(Stage::PostProcess);
    auto [stage3, stage3Index] = mDisplayResource->GetCurrentFrame(backendIndex, Stage::PostProcess, stage1Index);
    backend->IncrementFrameIndex();

    // =============================== Stage 1 DeferredRendering ===============================
    stage1->FlushDirect();
    stage1->ResetDirect();
    backend->DirectQueue->Wait(stage1->SharedFence.Get(), stage1->SharedFenceValue);
    stage1->DirectCmdList->RSSetViewports(1, &mViewPort);
    stage1->DirectCmdList->RSSetScissorRects(1, &mScissor);

    auto entitiesCB = stage1->mSceneCB.EntityCB->resource()->GetGPUVirtualAddress();
    auto &gbufferPass = *(backend->mGBufferPass);
    {
        gbufferPass.BeginPass(stage1->DirectCmdList.Get());
        stage1->SetCurrentFrameCB();
        mScene->RenderScene(stage1->DirectCmdList.Get(), entitiesCB, &backend->mRenderItems);
        mScene->RenderSphere(stage1->DirectCmdList.Get(), entitiesCB, &backend->mRenderItems);
        gbufferPass.EndPass(stage1->DirectCmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    auto &lightPass = *(backend->mLightPass);
    {
        lightPass.BeginPass(stage1->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON);
        mScene->RenderQuad(stage1->DirectCmdList.Get(), entitiesCB, &backend->mRenderItems);
        lightPass.EndPass(stage1->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON);
    }
    stage1->SubmitDirect(backend->DirectQueue.Get());
    stage1->SignalDirect(backend->DirectQueue.Get());

    // ========================Stage 2 Copy Texture ========================
    stage2->FlushCopy(); // 等待上一帧拷贝完毕
    stage2->ResetCopy();
    backend->CopyQueue->Wait(stage2->Fence.Get(), stage2->FenceValue); // 等待上一帧渲染完毕

    {
        stage2->CopyCmdList->CopyResource(stage2->GetResource("LightCopy"), stage2->GetResource("Light"));
    }
    stage2->SubmitCopy(backend->CopyQueue.Get());
    stage2->SignalCopy(backend->CopyQueue.Get());

    // ========================Stage 3 PostProcess ========================
    stage3->FlushDirect();
    stage3->ResetDirect();
    mDisplayResource->DirectQueue->Wait(stage3->SharedFence.Get(), stage3Backend->SharedFenceValue);
    stage3->DirectCmdList->RSSetViewports(1, &mViewPort);
    stage3->DirectCmdList->RSSetScissorRects(1, &mScissor);

    auto &sobelPass = *(mDisplayResource->mSobelPass);
    {
        sobelPass.BeginPass(stage3->DirectCmdList.Get());
        sobelPass.ExecutePass(stage3->DirectCmdList.Get());
        sobelPass.EndPass(stage3->DirectCmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    auto &quadPass = *(mDisplayResource->mQuadPass);
    {
        quadPass.BeginPass(stage3->DirectCmdList.Get());
        mScene->RenderScreenQuad(stage3->DirectCmdList.Get(), &mDisplayResource->ScreenQuadVBView, &mDisplayResource->ScreenQuadIBView);
        quadPass.EndPass(stage3->DirectCmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    GResource::GUIManager->DrawUI(stage3->DirectCmdList.Get(), stage3->GetResource("SwapChain"));

    stage3->SubmitDirect(mDisplayResource->DirectQueue.Get());
    stage3->SignalDirect(mDisplayResource->DirectQueue.Get());

    mDisplayResource->SwapChain->Present(0, 0);
}

void StageBeacon::SyncSingleExecutePass(BackendResource *backend, uint backendIndex)
{
    auto [stage, stageIndex] = mBackendResource[backendIndex]->GetCurrentFrame(Stage::DeferredRendering);

    stage->ResetDirect();
    backend->DirectQueue->Wait(stage->SharedFence.Get(), stage->SharedFenceValue);
    stage->DirectCmdList->RSSetViewports(1, &mViewPort);
    stage->DirectCmdList->RSSetScissorRects(1, &mScissor);

    auto entitiesCB = stage->mSceneCB.EntityCB->resource()->GetGPUVirtualAddress();
    auto &gbufferPass = *(backend->mGBufferPass);
    {
        gbufferPass.BeginPass(stage->DirectCmdList.Get());
        stage->SetCurrentFrameCB();
        mScene->RenderScene(stage->DirectCmdList.Get(), entitiesCB, &backend->mRenderItems);
        mScene->RenderSphere(stage->DirectCmdList.Get(), entitiesCB, &backend->mRenderItems);
        gbufferPass.EndPass(stage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    auto &lightPass = *(backend->mLightPass);
    {
        lightPass.BeginPass(stage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON);
        mScene->RenderQuad(stage->DirectCmdList.Get(), entitiesCB, &backend->mRenderItems);
        lightPass.EndPass(stage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON);
    }
    stage->SubmitDirect(backend->DirectQueue.Get());
    stage->SignalDirect(backend->DirectQueue.Get());

    stage->ResetCopy();
    backend->CopyQueue->Wait(stage->Fence.Get(), stage->FenceValue); // 等待上一帧渲染完毕

    {
        stage->CopyCmdList->CopyResource(stage->GetResource("LightCopy"), stage->GetResource("Light"));
    }
    stage->SubmitCopy(backend->CopyQueue.Get());
    stage->SignalCopy(backend->CopyQueue.Get());

    auto [displayStage, frameIndex] = mDisplayResource->GetCurrentSingleFrame(backendIndex, Stage::PostProcess, stageIndex);

    // stage->FlushDirect();
    displayStage->ResetDirect();
    mDisplayResource->DirectQueue->Wait(displayStage->SharedFence.Get(), stage->SharedFenceValue);
    displayStage->DirectCmdList->RSSetViewports(1, &mViewPort);
    displayStage->DirectCmdList->RSSetScissorRects(1, &mScissor);

    auto &sobelPass = *(mDisplayResource->mSobelPass);
    {
        sobelPass.BeginPass(displayStage->DirectCmdList.Get());
        sobelPass.ExecutePass(displayStage->DirectCmdList.Get());
        sobelPass.EndPass(displayStage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    auto &quadPass = *(mDisplayResource->mQuadPass);
    {
        quadPass.BeginPass(displayStage->DirectCmdList.Get());
        mScene->RenderScreenQuad(displayStage->DirectCmdList.Get(), &mDisplayResource->ScreenQuadVBView, &mDisplayResource->ScreenQuadIBView);
        quadPass.EndPass(displayStage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    GResource::GUIManager->DrawUI(displayStage->DirectCmdList.Get(), displayStage->GetResource("SwapChain"));

    displayStage->SubmitDirect(mDisplayResource->DirectQueue.Get());
    displayStage->SignalDirect(mDisplayResource->DirectQueue.Get());
    displayStage->FlushDirect();

    mDisplayResource->SwapChain->Present(0, 0);

    backend->IncrementFrameIndex();
    IncrementBackendIndex();
}
void StageBeacon::InitPipelineFunction()
{
    using TaskInfo = StagePipeline::TaskInfo;
    auto f1 = [this](TaskInfo info) {
        auto stage = &(mBackendResource[info.backendIndex]->mSFR[info.frameIndex]);
        auto &backend = mBackendResource[info.backendIndex];

        stage->ResetDirect();
        backend->DirectQueue->Wait(stage->SharedFence.Get(), stage->SharedFenceValue);
        stage->DirectCmdList->RSSetViewports(1, &mViewPort);
        stage->DirectCmdList->RSSetScissorRects(1, &mScissor);

        auto entitiesCB = stage->mSceneCB.EntityCB->resource()->GetGPUVirtualAddress();
        auto &gbufferPass = info.gBufferPass;
        {
            gbufferPass.BeginPass(stage->DirectCmdList.Get());
            stage->SetCurrentFrameCB();
            mScene->RenderScene(stage->DirectCmdList.Get(), entitiesCB, &backend->mRenderItems);
            mScene->RenderSphere(stage->DirectCmdList.Get(), entitiesCB, &backend->mRenderItems);
            gbufferPass.EndPass(stage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        auto &lightPass = info.lightPass;
        {
            lightPass.BeginPass(stage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON);
            mScene->RenderQuad(stage->DirectCmdList.Get(), entitiesCB, &backend->mRenderItems);
            lightPass.EndPass(stage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_COMMON);
        }
        stage->SubmitDirect(backend->DirectQueue.Get());
        stage->SignalDirect(backend->DirectQueue.Get());
        // stage->FlushDirect();
    };
    auto f2 = [this](TaskInfo info) {
        auto stage = &(mBackendResource[info.backendIndex]->mSFR[info.frameIndex]);
        auto &backend = mBackendResource[info.backendIndex];

        stage->ResetCopy();
        backend->CopyQueue->Wait(stage->Fence.Get(), stage->FenceValue); // 等待上一帧渲染完毕

        {
            stage->CopyCmdList->CopyResource(stage->GetResource("LightCopy"), stage->GetResource("Light"));
        }
        stage->SubmitCopy(backend->CopyQueue.Get());
        stage->SignalCopy(backend->CopyQueue.Get());
        // stage->FlushCopy(); // 等待上一帧拷贝完毕
    };
    auto f3 = [this](TaskInfo info) {
        auto [stage, stageIndex] = mDisplayResource->GetCurrentSingleFrame(info.backendIndex, Stage::PostProcess, info.frameIndex);

        // stage->FlushDirect();
        stage->ResetDirect();
        mDisplayResource->DirectQueue->Wait(stage->SharedFence.Get(), info.copyFenceValue);
        stage->DirectCmdList->RSSetViewports(1, &mViewPort);
        stage->DirectCmdList->RSSetScissorRects(1, &mScissor);

        auto &sobelPass = info.sobelPass;
        {
            sobelPass.BeginPass(stage->DirectCmdList.Get());
            sobelPass.ExecutePass(stage->DirectCmdList.Get());
            sobelPass.EndPass(stage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        auto &quadPass = info.quadPass;
        {
            quadPass.BeginPass(stage->DirectCmdList.Get());
            mScene->RenderScreenQuad(stage->DirectCmdList.Get(), &mDisplayResource->ScreenQuadVBView, &mDisplayResource->ScreenQuadIBView);
            quadPass.EndPass(stage->DirectCmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        GResource::GUIManager->DrawUI(stage->DirectCmdList.Get(), stage->GetResource("SwapChain"));

        stage->SubmitDirect(mDisplayResource->DirectQueue.Get());
        stage->SignalDirect(mDisplayResource->DirectQueue.Get());
        stage->FlushDirect();

        mDisplayResource->SwapChain->Present(0, 0);
    };
    mStagePipeline.SetStageTask(static_cast<uint>(Stage::DeferredRendering), f1);
    mStagePipeline.SetStageTask(static_cast<uint>(Stage::CopyTexture), f2);
    mStagePipeline.SetStageTask(static_cast<uint>(Stage::PostProcess), f3);
}

void StageBeacon::AsyncExecutePass(BackendResource *backend, uint backendIndex)
{
    if (mStagePipeline.GetWorkNum() < 1) {

        GResource::CPUTimerManager->UpdateTimer("RenderTime");
        GResource::CPUTimerManager->BeginTimer("DrawCall");
        using TaskInfo = StagePipeline::TaskInfo;
        auto [backend, backendIndex] = GetCurrentBackend();
        auto [stageResourc, stageIndex] = backend->GetCurrentFrame(Stage::DeferredRendering);
        TaskInfo info{
            backendIndex,
            stageIndex,
            stageResourc->SharedFenceValue + 1,
            *(backend->mGBufferPass),
            *(backend->mLightPass),
            *(mDisplayResource->mSobelPass),
            *(mDisplayResource->mQuadPass)};
        mStagePipeline.SubmitResource(info);

        backend->IncrementFrameIndex();
        IncrementBackendIndex();
        GResource::CPUTimerManager->EndTimer("DrawCall");
    }
}