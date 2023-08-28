#include "Beacon.h"
#include "Framework/Application.h"
#include "Tools/FrameworkHelper.h"
#include "DataStruct.h"
#include "GpuEntryLayout.h"
// #include "pix3/pix3.h"
Beacon::Beacon(uint width, uint height, std::wstring title) :
    RendererBase(width, height, title),
    mViewPort(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))

{
}

void Beacon::OnInit()
{
    HWND handle = Application::GetHandle();
    CreateDevice(handle);
    CreateCommandQueue();
    CreateSwapChain(handle);

    mResourceRegister = std::make_unique<ResourceRegister>(mDevice.Get());
    for (uint i = 0; i < mFrameCount; i++) {
        FrameResource frameResource(mResourceRegister.get());
        frameResource.Init(mDevice.Get());
        mFR.push_back(std::move(frameResource));
    }

    mFR.at(0).Reset();
    CompileShaders();
    CreateSignature2PSO();
    CreatePass();

    CreateRTV(mDevice.Get(), mSwapChain.Get(), mFrameCount);
    GResource::GUIManager->Init(mDevice.Get());
    GResource::GPUTimer = std::make_unique<D3D12GpuTimer>(mDevice.Get(), mCommandQueue.Get(), static_cast<UINT>(GpuTimers::NumTimers));
    GResource::GPUTimer->SetTimerName(static_cast<UINT>(GpuTimers::FPS), "Render ms");
    GResource::GPUTimer->SetTimerName(static_cast<uint>(GpuTimers::stage1), "beacon stage1 ms");
    GResource::GPUTimer->SetTimerName(static_cast<uint>(GpuTimers::stage3), "Beacon stage3 ms");

    LoadScene();

    for (auto &item : mFR) {
        item.CreateConstantBuffer(mDevice.Get(), mScene->GetEntityCount(), mScene->GetMaterialCount(), 1);
    }

    // Upload Committed Resource 0 - > 1
    mFR.at(0).Signal(mCommandQueue.Get());
    mFR.at(0).Sync();
}

void Beacon::OnRender()
{
    GResource::CPUTimerManager->UpdateTimer("RenderTime");
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

    int frameIndex = mCurrentBackBuffer;

    // ============================= Init Stage =================================
    GResource::GPUTimer->BeginTimer(mFR.at(frameIndex).CmdList.Get(), static_cast<uint>(GpuTimers::FPS));
    GResource::CPUTimerManager->BeginTimer("DrawCall");
    mFR.at(frameIndex).CmdList->RSSetViewports(1, &mViewPort);
    mFR.at(frameIndex).CmdList->RSSetScissorRects(1, &mScissor);

    SetPass(frameIndex);
    ExecutePass(frameIndex);

    // UI Pass
   
    GResource::GUIManager->DrawUI(mFR.at(frameIndex).CmdList.Get(), mFR.at(frameIndex).GetResource("SwapChain"),{});
    GResource::GPUTimer->EndTimer(mFR.at(frameIndex).CmdList.Get(), static_cast<uint>(GpuTimers::stage3));

    // ===============================End Stage ==================================
    GResource::CPUTimerManager->EndTimer("DrawCall");
    GResource::GPUTimer->EndTimer(mFR.at(frameIndex).CmdList.Get(), static_cast<std::uint32_t>(GpuTimers::FPS));
    GResource::GPUTimer->ResolveAllTimers(mFR.at(frameIndex).CmdList.Get());

    mFR.at(frameIndex).Signal(mCommandQueue.Get());

    mSwapChain->Present(0, 0);

    GetCurrentBackBuffer();
}

void Beacon::OnUpdate()
{
    uint frameIndex = mCurrentBackBuffer;

    mFR.at(frameIndex).Sync();
    mFR.at(frameIndex).Reset();

    mScene->UpdateCamera();
    mScene->UpdateSceneConstant(mFR.at(frameIndex).SceneConstant.get());
    mScene->UpdateEntityConstant(mFR.at(frameIndex).EntityConstant.get());
    mScene->UpdateLightConstant(mFR.at(frameIndex).LightConstant.get());
    mScene->UpdateMaterialConstant(mFR.at(frameIndex).MaterialConstant.get());
}

void Beacon::OnDestory()
{
    for (auto &item : mFR) {
        GResource::GPUTimer->ResolveAllTimers(item.CmdList.Get());
        mCommandQueue->Signal(item.Fence.Get(), ++item.FenceValue);
        item.Sync();
    }

    GResource::GPUTimer = nullptr;
    GResource::GPUTimer = nullptr;
    mScene = nullptr;
    mFR.clear();
    mQuadPass = nullptr;
    mResourceRegister.reset();
    mCommandQueue = nullptr;
    mSwapChain = nullptr;
    mDeviceAdapter = nullptr;
    mPSO.clear();
    mSignature.clear();
    mDevice = nullptr;
    mFactory = nullptr;
}
Beacon::~Beacon()
{
}
void Beacon::OnMouseDown(WPARAM btnState, int x, int y)
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

void Beacon::OnKeyDown(byte key)
{
}

void Beacon::CreateDevice(HWND handle)
{
    ComPtr<ID3D12Debug3> debugController;
    UINT dxgiFactoryFlags = 0;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));
    ThrowIfFailed(mFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES));

    DXGI_ADAPTER_DESC1 adapterDesc = {};
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != mFactory->EnumAdapters1(i, &mDeviceAdapter); i++) {
        mDeviceAdapter->GetDesc1(&adapterDesc);
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) continue;
        std::wstring str = adapterDesc.Description;

        if (str.find(L"1060 6G")!=std::string::npos && SUCCEEDED((D3D12CreateDevice(mDeviceAdapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))) {
            OutputDebugStringW(str.c_str());

            break;
        }
    }
    auto deviceName = adapterDesc.Description;
    ThrowIfFailed(D3D12CreateDevice(mDeviceAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));

    // ComPtr<ID3D12InfoQueue> infoQueue;
    // if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
    //     infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    //     infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    //     infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    //     infoQueue->GetMuteDebugOutput();
    // }
}

void Beacon::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
}

void Beacon::CreateSwapChain(HWND handle)
{
    ComPtr<IDXGISwapChain1> swapchain1;
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.Width = GetWidth();
    swapchainDesc.Height = GetHeight();
    swapchainDesc.BufferCount = mFrameCount;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.SampleDesc.Count = 1;

    ThrowIfFailed(mFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), handle, &swapchainDesc, nullptr, nullptr, &swapchain1));
    ThrowIfFailed(swapchain1.As(&mSwapChain));
}

void Beacon::CreateRTV(ID3D12Device *device, IDXGISwapChain4 *swapchain, uint frameCount)
{
    for (uint i = 0; i < frameCount; i++) {
        ComPtr<ID3D12Resource> buffer;
        ThrowIfFailed(swapchain->GetBuffer(i, IID_PPV_ARGS(&buffer)));
        mFR.at(i).CreateRenderTarget(mDevice.Get(), buffer.Get());
    }
}
void Beacon::LoadScene()
{
    // TODO add read config file
    auto path = GResource::config["Scene"]["ScenePath"].as<std::string>();
    auto sceneName = GResource::config["Scene"]["SceneName"].as<std::string>();
    auto scene = std::make_unique<Scene>(path, sceneName);
    scene->Init(mDevice.Get(), mFR.at(0).CmdList.Get(), mResourceRegister->SrvCbvUavDescriptorHeap.get());
    mEnvMapIndex = scene->GetEnvSrvIndex();
    mScene = std::move(scene);
}

void Beacon::CompileShaders()
{
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/GBuffer.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["GBufferVS"], &error));

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/GBuffer.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["GBufferPS"], &error));

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/LightingPass.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["LightPassVS"], &error));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/LightingPass.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["LightPassPS"], &error));

    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/PostProcess.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "SobelMain", "cs_5_1", compileFlags, 0, &GResource::Shaders["SobelCS"], &error));

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/ScreenQuad.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &GResource::Shaders["ScreenQuadPS"], &error));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/ScreenQuad.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &GResource::Shaders["ScreenQuadVS"], &error));
    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
}

int Beacon::GetCurrentBackBuffer()
{
    return mCurrentBackBuffer = (mCurrentBackBuffer + 1) % mFrameCount;
}

void Beacon::CreateSignature2PSO()
{
    GResource::InputLayout = GpuEntryLayout::CreateInputLayout();

    mSignature["Graphic"] = GpuEntryLayout::CreateRenderSignature(mDevice.Get(), GBufferPass::GetTargetCount());
    mSignature["Compute"] = GpuEntryLayout::CreateComputeSignature(mDevice.Get());

    mPSO["GBuffer"] = GpuEntryLayout::CreateGBufferPassPSO(mDevice.Get(), mSignature["Graphic"].Get(),
                                                           GBufferPass::GetTargetFormat(),
                                                           GBufferPass::GetDepthFormat());
    mPSO["LightPass"] = GpuEntryLayout::CreateLightPassPSO(mDevice.Get(), mSignature["Graphic"].Get());
    mPSO["SobelPass"] = GpuEntryLayout::CreateSobelPSO(mDevice.Get(), mSignature["Compute"].Get());
    mPSO["QuadPass"] = GpuEntryLayout::CreateMixQuadPassPSO(mDevice.Get(), mSignature["Graphic"].Get());
}

void Beacon::CreatePass()
{
    mGBufferPass = std::make_unique<GBufferPass>(mPSO["GBuffer"].Get(), mSignature["Graphic"].Get());

    mLightPass = std::make_unique<LightPass>(mPSO["LightPass"].Get(), mSignature["Graphic"].Get());

    mSobelPass = std::make_unique<SobelPass>(mPSO["SobelPass"].Get(), mSignature["Compute"].Get());

    mQuadPass = std::make_unique<MixQuadPass>(mPSO["QuadPass"].Get(), mSignature["Graphic"].Get());
}

void Beacon::SetPass(uint frameIndex)
{
    // ================== GBufferPass ==================

    std::vector<ID3D12Resource *> mutiRT;
    for (uint i = 0; i < GBufferPass::GetTargetCount(); i++) {
        std::string index = "GBuffer" + std::to_string(i);
        mutiRT.push_back(mFR.at(frameIndex).GetResource(index));
    }
    auto rtvHandle = mFR.at(frameIndex).GetRtv("GBuffer0");
    auto dsvHandle = mFR.at(frameIndex).GetDsv("Depth");
    mGBufferPass->SetRenderTarget(mutiRT, rtvHandle);
    mGBufferPass->SetDepthBuffer(mFR.at(frameIndex).GetResource("Depth"), dsvHandle);
    mGBufferPass->SetRTVDescriptorSize(mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

    // ================== LightPass ==================

    mLightPass->SetRenderTarget(mFR.at(frameIndex).GetResource("ScreenTexture1"), mFR.at(frameIndex).GetRtv("ScreenTexture1"));
    mLightPass->SetGBuffer(mFR.at(frameIndex).SrvCbvUavDescriptorHeap->Resource(), mFR.at(frameIndex).GetSrvCbvUav("GBuffer0"));
    mLightPass->SetTexture(mFR.at(frameIndex).SrvCbvUavDescriptorHeap->Resource(), mFR.at(frameIndex).SrvCbvUavDescriptorHeap->GPUHandle(0));
    mLightPass->SetCubeMap(mFR.at(0).SrvCbvUavDescriptorHeap->Resource(), mFR.at(0).SrvCbvUavDescriptorHeap->GPUHandle(mEnvMapIndex));

    // ================== SobelPass ==================
    mSobelPass->SetInput(mFR.at(frameIndex).GetSrvCbvUav("ScreenTexture1"));
    mSobelPass->SetSrvHeap(mFR.at(frameIndex).SrvCbvUavDescriptorHeap->Resource());
    mSobelPass->SetTarget(mFR.at(frameIndex).GetResource("ScreenTexture2"), mFR.at(frameIndex).GetSrvCbvUav("ScreenTexture2"));

    // ================== QuadPass ===================
    mQuadPass->SetTarget(mFR.at(frameIndex).GetResource("SwapChain"), mFR.at(frameIndex).GetRtv("SwapChain"));
    mQuadPass->SetRenderType(QuadShader::MixQuad);
    mQuadPass->SetSrvHandle(mFR.at(frameIndex).GetSrvCbvUav("ScreenTexture1"));
}
void Beacon::ExecutePass(uint frameIndex)
{
    auto constantAddress = mFR.at(frameIndex).EntityConstant->resource()->GetGPUVirtualAddress();
    // ===============================G-Buffer Pass===============================
    // PIXBeginEvent(mFR.at(frameIndex).CmdList.Get(), 0, L"GBufferPass");
    // {
    GResource::GPUTimer->BeginTimer(mFR.at(frameIndex).CmdList.Get(), static_cast<uint>(GpuTimers::stage1));

    mGBufferPass->BeginPass(mFR.at(frameIndex).CmdList.Get());
    mFR.at(frameIndex).SetSceneConstant();
    mScene->RenderScene(mFR.at(frameIndex).CmdList.Get(), constantAddress);
    mScene->RenderSphere(mFR.at(frameIndex).CmdList.Get(), constantAddress);
    mGBufferPass->EndPass(mFR.at(frameIndex).CmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);

    // }
    // PIXEndEvent(mFR.at(frameIndex).CmdList.Get());

    // ===============================Light Pass =================================

    mLightPass->BeginPass(mFR.at(frameIndex).CmdList.Get());
    for (uint i = 0; i < GResource::config["Scene"]["LightPassLoop"].as<uint>(); i++) {
        mScene->RenderQuad(mFR.at(frameIndex).CmdList.Get(), constantAddress);
    }
    mLightPass->EndPass(mFR.at(frameIndex).CmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);

    GResource::GPUTimer->EndTimer(mFR.at(frameIndex).CmdList.Get(), static_cast<uint>(GpuTimers::stage1));
    // =============================== Sobel Pass ================================
     GResource::GPUTimer->BeginTimer(mFR.at(frameIndex).CmdList.Get(), static_cast<uint>(GpuTimers::stage3));

    mSobelPass->BeginPass(mFR.at(frameIndex).CmdList.Get());





    mSobelPass->EndPass(mFR.at(frameIndex).CmdList.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);

    // ============================= Screen Quad Pass ===========================

    mQuadPass->BeginPass(mFR.at(frameIndex).CmdList.Get());
    mScene->RenderQuad(mFR.at(frameIndex).CmdList.Get(), constantAddress);
    mQuadPass->EndPass(mFR.at(frameIndex).CmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
}
