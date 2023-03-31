#include "Beacon.h"
#include "Framework/Application.h"
#include "Tools/FrameworkHelper.h"
#include "DataStruct.h"

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
    CreateCommandResource();
    CreateSwapChain(handle);
    CreateFence();

    GResource::TextureManager = std::make_unique<TextureManager>(mDevice.Get(), 1000);
    CreateRTV(mDevice.Get(), mSwapChain.Get(), mFrameCount);
    GResource::GUIManager->Init(mDevice.Get());
    GResource::GPUTimer = std::make_unique<D3D12GpuTimer>(mDevice.Get(), mCommandQueue.Get(), static_cast<UINT>(GpuTimers::NumTimers));
    GResource::GPUTimer->SetTimerName(static_cast<UINT>(GpuTimers::FPS), "render ms");

    LoadScene();

    mDeferredRendering = std::make_unique<DeferredRendering>(mWidth, mHeight);
    mDeferredRendering->Init(mDevice.Get());

    mQuadPass = std::make_unique<ScreenQuad>();
    mQuadPass->Init(mDevice.Get(), mCommandList.Get());

    mPostProcesser = std::make_unique<SobelFilter>();
    mPostProcesser->Init(mDevice.Get());

    // Upload Committed Resource 0 - > 1
    mCommandList->Close();
    std::array<ID3D12CommandList *, 1> taskList = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(taskList.size(), taskList.data());
    SyncTask();
}

void Beacon::OnRender()
{
    GResource::CPUTimerManager->UpdateTimer("RenderTime");

    int frameIndex = mSwapChain->GetCurrentBackBufferIndex();
    mCommandAllocator->Reset();
    mCommandList->Reset(mCommandAllocator.Get(), nullptr);

    // ============================= Init Stage =================================
    GResource::GPUTimer->BeginTimer(mCommandList.Get(), static_cast<std::uint32_t>(GpuTimers::FPS));
    GResource::CPUTimerManager->BeginTimer("DrawCall");
    mCommandList->RSSetViewports(1, &mViewPort);
    mCommandList->RSSetScissorRects(1, &mScissor);

    // ===============================G-Buffer Pass===============================
    mDeferredRendering->GBufferPass(mCommandList.Get());
    mScene->RenderScene(mCommandList.Get());

    // ===============================Light Pass =================================
    auto rtvHandle = mRTVDescriptorHeap->CPUHandle(mFrameCount);
    // auto rtvHandle = mRTVDescriptorHeap->CPUHandle(frameIndex);
    mDeferredRendering->LightPass(mCommandList.Get());
    auto *mediaTexture1 = GResource::TextureManager->GetTexture(mMediaRTVBuffer.at(0))->Resource();
    auto *srvHeap = GResource::TextureManager->GetTexture2DDescriptoHeap();
    auto srv2rtv = CD3DX12_RESOURCE_BARRIER::Transition(mediaTexture1,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET);
    // auto present2rtv0 = CD3DX12_RESOURCE_BARRIER::Transition(mRTVBuffer.at(frameIndex).Get(),
    //                                                          D3D12_RESOURCE_STATE_PRESENT,
    //                                                          D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &srv2rtv);
    // mCommandList->ResourceBarrier(1,&present2rtv0);
    mCommandList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);
    float clearValue[] = {0, 0, 0, 1.0F};
    mCommandList->ClearRenderTargetView(rtvHandle, clearValue, 0, nullptr);

    mQuadPass->Draw(mCommandList.Get());
    auto rtv2srv = CD3DX12_RESOURCE_BARRIER::Transition(mediaTexture1,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ);
    mCommandList->ResourceBarrier(1, &rtv2srv);
    // =============================== Sobel Pass ================================
    mPostProcesser->Draw(mCommandList.Get(), srvHeap->GPUHandle(mDeferredRendering->GetDepthTexture()));
    auto *sobelTexture = mPostProcesser->OuptputResource();
    // ============================= Screen Quad Pass ===========================
    auto present2rtv = CD3DX12_RESOURCE_BARRIER::Transition(mRTVBuffer.at(frameIndex).Get(),
                                                            D3D12_RESOURCE_STATE_PRESENT,
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &present2rtv);
    rtvHandle = mRTVDescriptorHeap->CPUHandle(frameIndex);
    mCommandList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);
    mCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::SteelBlue, 0, nullptr);
    bool isEnablePostProcess = false;
    if (!isEnablePostProcess) {
        mQuadPass->SetState(mCommandList.Get(), QuadShader::MixQuad);
        auto srv1Handle = srvHeap->GPUHandle(mMediaSrvIndex.at(0));
        auto srv2Handle = srvHeap->GPUHandle(mPostProcesser->OutputSrvIndex());
        mCommandList->SetGraphicsRootDescriptorTable(1, srv1Handle);
        mCommandList->SetGraphicsRootDescriptorTable(2, srv2Handle);
        mQuadPass->Draw(mCommandList.Get());
    } else {
        mQuadPass->Draw(mCommandList.Get());
    }

    // ===============================UI Pass ====================================

    GResource::GUIManager->DrawUI(mCommandList.Get(), mRTVBuffer.at(frameIndex).Get());

    // ===============================End Stage ==================================
    GResource::CPUTimerManager->EndTimer("DrawCall");
    GResource::GPUTimer->EndTimer(mCommandList.Get(), static_cast<std::uint32_t>(GpuTimers::FPS));
    GResource::GPUTimer->ResolveAllTimers(mCommandList.Get());

    mCommandList->Close();
    std::array<ID3D12CommandList *, 1> taskList = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(taskList.size(), taskList.data());

    mSwapChain->Present(0, 0);
    SyncTask();
}

void Beacon::OnUpdate()
{
    mScene->UpdateScene();
}

void Beacon::OnDestory()
{
    mScene.reset();
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
        // debugController->SetEnableSynchronizedCommandQueueValidation(TRUE);
        // debugController->SetEnableGPUBasedValidation(TRUE);
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));
    ThrowIfFailed(mFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES));

    DXGI_ADAPTER_DESC1 adapterDesc = {};
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != mFactory->EnumAdapters1(i, &mDeviceAdapter); i++) {
        mDeviceAdapter->GetDesc1(&adapterDesc);
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) continue;
        std::wstring str = adapterDesc.Description;
        OutputDebugStringW(str.c_str());
        if (i == 1 && SUCCEEDED(D3D12CreateDevice(mDeviceAdapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr))) break;
    }
    auto deviceName = adapterDesc.Description;
    ThrowIfFailed(D3D12CreateDevice(mDeviceAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));

    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        infoQueue->GetMuteDebugOutput();
    }
}

void Beacon::CreateCommandResource()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
    ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
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
    uint mediaRTVNum = 1;
    mRTVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, frameCount + mediaRTVNum);
    for (UINT i = 0; i < frameCount; i++) {
        ComPtr<ID3D12Resource> buffer;
        auto handle = mRTVDescriptorHeap->CPUHandle(i);
        ThrowIfFailed(swapchain->GetBuffer(i, IID_PPV_ARGS(&buffer)));
        device->CreateRenderTargetView(buffer.Get(), nullptr, handle);
        mRTVBuffer.push_back(std::move(buffer));
    }

    uint rtvIndex = frameCount;
    // Middle Texture
    for (uint i = 0; i < mediaRTVNum; i++) {
        Texture texture(device, DXGI_FORMAT_R8G8B8A8_UNORM, mWidth, mHeight, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        texture.Resource()->SetName(L"PassMiddleTexture");
        auto handle = mRTVDescriptorHeap->CPUHandle(rtvIndex);
        device->CreateRenderTargetView(texture.Resource(), nullptr, handle);
        uint resourceIndex = GResource::TextureManager->StoreTexture(texture);
        uint srvIndex = GResource::TextureManager->AddSrvDescriptor(resourceIndex, DXGI_FORMAT_R8G8B8A8_UNORM);
        mMediaRTVBuffer.push_back(resourceIndex);
        mMediaSrvIndex.push_back(srvIndex);
        rtvIndex++;
    }
}
void Beacon::LoadScene()
{
    // TODO add read config file
    std::wstring Path = L"C:\\Users\\Geek\\Desktop\\dst";
    auto scene = std::make_unique<Scene>(Path, L"witch");

    SceneAdapter adapater{
        .Device = mDevice.Get(),
        .CommandList = mCommandList.Get(),
        .SwapChain = mSwapChain.Get(),
        .FrameWidth = mWidth,
        .FrameHeight = mHeight,
        .FrameCount = mFrameCount};
    scene->Init(adapater);

    mScene = std::move(scene);
}

void Beacon::CreateFence()
{
    ThrowIfFailed(mDevice->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFenceValue = 0;
}

void Beacon::SyncTask()
{
    mFenceValue++;
    mCommandQueue->Signal(mFence.Get(), mFenceValue);

    if (mFence->GetCompletedValue() < mFenceValue) {
        HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);

        CloseHandle(fenceEvent);
    }
}