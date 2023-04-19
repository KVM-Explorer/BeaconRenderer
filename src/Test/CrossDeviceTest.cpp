#include <stdafx.h>
#include "Tools/FrameworkHelper.h"
#include <iostream>
#include <format>

ComPtr<IDXGIFactory5> Factory;
ComPtr<IDXGIAdapter1> mDeviceAdapter;
ComPtr<ID3D12Device> iGPU;
ComPtr<ID3D12Device> dGPU;

ComPtr<ID3D12CommandQueue> iGpu3DQueue;
ComPtr<ID3D12CommandQueue> dGpu3DQueue;
ComPtr<ID3D12CommandQueue> dGpuCopyQueue;

ComPtr<ID3D12CommandAllocator> dGpuCmdAllocator;
ComPtr<ID3D12CommandAllocator> dGpuCopyCmdAllocator;
ComPtr<ID3D12GraphicsCommandList> dGpuCmdList;
ComPtr<ID3D12GraphicsCommandList> dGpuCopyCmdList;
ComPtr<ID3D12CommandAllocator> iGpuCmdAllocator;
ComPtr<ID3D12GraphicsCommandList> iGpuCmdList;

ComPtr<IDXGISwapChain4> SwapChain;

ComPtr<ID3D12DescriptorHeap> iGpuRtvHeap;
ComPtr<ID3D12DescriptorHeap> dGpuRtvHeap;
ComPtr<ID3D12DescriptorHeap> iGpuSrvHeap;

std::unordered_map<std::string, ComPtr<ID3D12Resource>> iGpuResourceMap;
std::unordered_map<std::string, ComPtr<ID3D12Resource>> dGpuResourceMap;

uint iGpuRtvDescriptorSize;
uint dGpuRtvDescriptorSize;
uint iGpuSrvDescriptorSize;
uint dGpuSrvDescriptorSize;

D3D12_VERTEX_BUFFER_VIEW iGpuVertexBufferView;

ComPtr<ID3D12RootSignature> iGpuRootSignature;

ComPtr<ID3DBlob> iGpuVS;
ComPtr<ID3DBlob> iGpuPS;

ComPtr<ID3D12PipelineState> iGpuPSO;

ComPtr<ID3D12Fence> dGpuFence;
ComPtr<ID3D12Fence> iGpuFence;
ComPtr<ID3D12Fence> iGpuSharedFence;
ComPtr<ID3D12Fence> dGpuSharedFence;
HANDLE iGpuSharedFenceEvent;

uint64 mFenceValue = 0;
const uint mFrameCount = 3;
const uint mWidth = 600;
const uint mHeight = 600;

D3D12_VIEWPORT ViewPort = {0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight)};
D3D12_RECT ScissorRect = {0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight)};

HWND WindowHandle;

LRESULT CALLBACK WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam);
void OnRender();

void FlushCommandQueue(ID3D12CommandQueue *commandQueue, ID3D12Fence *fence, uint64_t &fenceValue)
{
    auto tmp = fence->GetCompletedValue();
    if (fence->GetCompletedValue() < fenceValue) {
        HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    }
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    ComPtr<ID3D12Debug3> debugController;
    UINT dxgiFactoryFlags = 0;

    // 0. Create Window
    {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_GLOBALCLASS;
        wcex.lpfnWndProc = WindowProc;
        wcex.hInstance = hInstance;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.lpszClassName = L"CrossDeviceTest";
        wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        RegisterClassEx(&wcex);

        RECT windowRect = {0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight)};
        AdjustWindowRect(&windowRect, WS_OVERLAPPED | WS_SYSMENU, FALSE);

        WindowHandle = CreateWindow(
            wcex.lpszClassName,
            L"CrossDeviceTest",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,
            nullptr,
            wcex.hInstance,
            nullptr);
    }

    // 1. Create Device
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    ComPtr<IDXGIFactory2> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
    ThrowIfFailed(factory.As(&Factory));

    DXGI_ADAPTER_DESC1 adapterDesc = {};
    ComPtr<IDXGIOutput> tmpOutput;
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(i, &mDeviceAdapter); i++) {
        mDeviceAdapter->GetDesc1(&adapterDesc);
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) continue;
        std::wstring str = adapterDesc.Description;

        auto hr = mDeviceAdapter->EnumOutputs(0, &tmpOutput);
        if (SUCCEEDED(hr) && tmpOutput != nullptr) {
            auto outputStr = std::format(L"iGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
            std::wcout << outputStr;
            ThrowIfFailed(D3D12CreateDevice(mDeviceAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&iGPU)));
        } else {
            auto outputStr = std::format(L"dGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
            std::wcout << outputStr;
            ThrowIfFailed(D3D12CreateDevice(mDeviceAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&dGPU)));
        }
    }

    // Check Support
    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    iGPU->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

    std::cout << "CrossAdapterRowMajorTexture" << ' ' << options.CrossAdapterRowMajorTextureSupported << std::endl;

    // 2. Create CommandQueue
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ThrowIfFailed(iGPU->CreateCommandQueue(&desc, IID_PPV_ARGS(&iGpu3DQueue)));
        ThrowIfFailed(dGPU->CreateCommandQueue(&desc, IID_PPV_ARGS(&dGpu3DQueue)));
        desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        ThrowIfFailed(dGPU->CreateCommandQueue(&desc, IID_PPV_ARGS(&dGpuCopyQueue)));

        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(dGPU->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
            infoQueue->GetMuteDebugOutput();
        }
        if (SUCCEEDED(iGPU->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
            infoQueue->GetMuteDebugOutput();
        }
    }
    // 3. Create CommandList
    {
        ThrowIfFailed(dGPU->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&dGpuCmdAllocator)));
        ThrowIfFailed(dGPU->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, dGpuCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&dGpuCmdList)));

        ThrowIfFailed(dGPU->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&dGpuCopyCmdAllocator)));
        ThrowIfFailed(dGPU->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, dGpuCopyCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&dGpuCopyCmdList)));

        ThrowIfFailed(iGPU->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&iGpuCmdAllocator)));
        ThrowIfFailed(iGPU->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, iGpuCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&iGpuCmdList)));
    }
    // 4. Create SwapChain
    {
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = mWidth;
        swapchainDesc.Height = mHeight;
        swapchainDesc.BufferCount = mFrameCount;
        swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(Factory->CreateSwapChainForHwnd(iGpu3DQueue.Get(), WindowHandle, &swapchainDesc, nullptr, nullptr, &swapChain));

        ThrowIfFailed(swapChain.As(&SwapChain));
        ThrowIfFailed(Factory->MakeWindowAssociation(WindowHandle, DXGI_MWA_NO_ALT_ENTER));
    }
    // 5. Create DescriptorHeap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = 10;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ThrowIfFailed(iGPU->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&iGpuRtvHeap)));
        ThrowIfFailed(dGPU->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dGpuRtvHeap)));
        iGpuRtvDescriptorSize = iGPU->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        dGpuRtvDescriptorSize = dGPU->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(iGPU->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&iGpuSrvHeap)));
        iGpuSrvDescriptorSize = iGPU->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    // 6. Create Texture
    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width = mWidth;
        desc.Height = mHeight;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        clearValue.Color[0] = DirectX::Colors::LightSteelBlue[0];
        clearValue.Color[1] = DirectX::Colors::LightSteelBlue[1];
        clearValue.Color[2] = DirectX::Colors::LightSteelBlue[2];
        clearValue.Color[3] = DirectX::Colors::LightSteelBlue[3];

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        ThrowIfFailed(dGPU->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, &clearValue, IID_PPV_ARGS(&dGpuResourceMap["Render"])));
        dGpuResourceMap["Render"]->SetName(L"dGpuRender");

        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ThrowIfFailed(dGPU->CreateCommittedResource(&heapProps,
                                                    D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER | D3D12_HEAP_FLAG_SHARED,
                                                    &desc,
                                                    D3D12_RESOURCE_STATE_COPY_DEST,
                                                    nullptr,
                                                    IID_PPV_ARGS(&dGpuResourceMap["Copy"])));
        dGpuResourceMap["Copy"]->SetName(L"dGpuCopy");

        ThrowIfFailed(iGPU->CreateCommittedResource(&heapProps,
                                                    D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER | D3D12_HEAP_FLAG_SHARED,
                                                    &desc,
                                                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                    nullptr,
                                                    IID_PPV_ARGS(&iGpuResourceMap["Copy"])));
        iGpuResourceMap["Copy"]->SetName(L"iGpuCopy");
    }

    // 7. Create Render Target View
    {
        D3D12_CPU_DESCRIPTOR_HANDLE iGpuRtvHandle = iGpuRtvHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_CPU_DESCRIPTOR_HANDLE dGpuRtvHandle = dGpuRtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (int i = 0; i < mFrameCount; i++) {
            auto name = "SwapChain" + std::to_string(i);
            ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&iGpuResourceMap[name])));
            iGPU->CreateRenderTargetView(iGpuResourceMap[name].Get(), nullptr, iGpuRtvHandle);
            iGpuRtvHandle.ptr += iGpuRtvDescriptorSize;
        }

        dGPU->CreateRenderTargetView(dGpuResourceMap["Render"].Get(), nullptr, dGpuRtvHandle);
        dGpuRtvHandle.ptr += dGpuRtvDescriptorSize;
    }
    // 8. Create Shader Resource View
    {
        D3D12_CPU_DESCRIPTOR_HANDLE iGpuSrvHandle = iGpuSrvHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MipLevels = 1;

        iGPU->CreateShaderResourceView(iGpuResourceMap["Copy"].Get(), &desc, iGpuSrvHandle);
        iGpuSrvHandle.ptr += iGpuSrvDescriptorSize;
    }
    // 9. Create Quad Vertex
    {
        struct Vertex {
            float x, y, z;
            float u, v;
        };

        Vertex vertices[] = {
            {-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 0.0f, 1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
            {1.0f, -1.0f, 0.0f, 1.0f, 1.0f},
        };

        const UINT vertexBufferSize = sizeof(vertices);

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

        ThrowIfFailed(iGPU->CreateCommittedResource(&heapProps,
                                                    D3D12_HEAP_FLAG_NONE,
                                                    &resourceDesc,
                                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                                    nullptr,
                                                    IID_PPV_ARGS(&iGpuResourceMap["QuadVB"])));

        UINT8 *pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(iGpuResourceMap["QuadVB"]->Map(0, &readRange, reinterpret_cast<void **>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, vertices, sizeof(vertices));
        iGpuResourceMap["QuadVB"]->Unmap(0, nullptr);

        iGpuVertexBufferView.BufferLocation = iGpuResourceMap["QuadVB"]->GetGPUVirtualAddress();
        iGpuVertexBufferView.StrideInBytes = sizeof(Vertex);
        iGpuVertexBufferView.SizeInBytes = vertexBufferSize;
    }
    // 10. Shader & Signature & PSO

    // 10.1. Create Root Signature
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC samplers;
        samplers.Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(iGPU->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&iGpuRootSignature)));
    }
    // 10.2. Create Shader
    {
        ComPtr<ID3DBlob> error;
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        ThrowIfFailed(D3DCompileFromFile(L"Shaders/QuadTest.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &iGpuVS, &error));
        ThrowIfFailed(D3DCompileFromFile(L"Shaders/QuadTest.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &iGpuPS, &error));
    }
    // 10.3 Create PSO
    {
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
        desc.pRootSignature = iGpuRootSignature.Get();
        desc.VS = CD3DX12_SHADER_BYTECODE(iGpuVS.Get());
        desc.PS = CD3DX12_SHADER_BYTECODE(iGpuPS.Get());
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        desc.DepthStencilState.DepthEnable = FALSE;
        desc.DepthStencilState.StencilEnable = FALSE;
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;

        ThrowIfFailed(iGPU->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&iGpuPSO)));
    }

    // 11. Create Fence
    {
        ThrowIfFailed(iGPU->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&iGpuFence)));
        ThrowIfFailed(dGPU->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&dGpuFence)));

        ThrowIfFailed(dGPU->CreateFence(0, D3D12_FENCE_FLAG_SHARED | D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER,
                                        IID_PPV_ARGS(&dGpuSharedFence)));

        ThrowIfFailed(dGPU->CreateSharedHandle(dGpuSharedFence.Get(), nullptr, GENERIC_ALL, nullptr, &iGpuSharedFenceEvent));

        HRESULT hrOpenSharedHandleResult = iGPU->OpenSharedHandle(iGpuSharedFenceEvent, IID_PPV_ARGS(&iGpuSharedFence));
        ::CloseHandle(iGpuSharedFenceEvent);
        ThrowIfFailed(hrOpenSharedHandleResult);
    }
    // Convert Resource State
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(dGpuResourceMap["Render"].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON);
        dGpuCmdList->ResourceBarrier(1, &barrier);
    }
    // 12. Sync
    {
        dGpuCmdList->Close();
        iGpuCmdList->Close();
        dGpuCopyCmdList->Close();

        ID3D12CommandList *iGpuCommandLists[] = {iGpuCmdList.Get()};
        ID3D12CommandList *dGpuCommandLists[] = {dGpuCmdList.Get()};

        dGpu3DQueue->ExecuteCommandLists(1, dGpuCommandLists);
        iGpu3DQueue->ExecuteCommandLists(1, iGpuCommandLists);
        dGpu3DQueue->Signal(dGpuFence.Get(), ++mFenceValue);
        if (dGpuFence->GetCompletedValue() < mFenceValue) {
            HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
            ThrowIfFailed(dGpuFence->SetEventOnCompletion(mFenceValue, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
            CloseHandle(fenceEvent);
        }
        iGpu3DQueue->Signal(iGpuFence.Get(), ++mFenceValue);
        if (iGpuFence->GetCompletedValue() < mFenceValue) {
            HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
            ThrowIfFailed(iGpuFence->SetEventOnCompletion(mFenceValue, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
            CloseHandle(fenceEvent);
        }
    }

    ShowWindow(WindowHandle, nCmdShow);
    UpdateWindow(WindowHandle);
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    FlushCommandQueue(iGpu3DQueue.Get(), iGpuFence.Get(), mFenceValue);

    return static_cast<char>(msg.wParam);
}



void OnRender()
{
    uint frameIndex = SwapChain->GetCurrentBackBufferIndex();
    auto swapChainName = "SwapChain" + std::to_string(frameIndex);

    dGpuCmdAllocator->Reset();
    dGpuCmdList->Reset(dGpuCmdAllocator.Get(), nullptr);
    dGpuCmdList->RSSetViewports(1, &ViewPort);
    dGpuCmdList->RSSetScissorRects(1, &ScissorRect);

    // ======================================= Render =======================================
    auto read2rtv = CD3DX12_RESOURCE_BARRIER::Transition(dGpuResourceMap["Render"].Get(), D3D12_RESOURCE_STATE_COMMON,
                                                         D3D12_RESOURCE_STATE_RENDER_TARGET);
    dGpuCmdList->ResourceBarrier(1, &read2rtv);
    auto dGpuRtvHandle = dGpuRtvHeap->GetCPUDescriptorHandleForHeapStart();
    dGpuCmdList->OMSetRenderTargets(1, &dGpuRtvHandle, FALSE, nullptr);
    dGpuCmdList->ClearRenderTargetView(dGpuRtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);

    auto rtv2read = CD3DX12_RESOURCE_BARRIER::Transition(dGpuResourceMap["Render"].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                         D3D12_RESOURCE_STATE_COMMON);
    dGpuCmdList->ResourceBarrier(1, &rtv2read);
    dGpuCmdList->Close();

    // ========================================= Copy ========================================
    dGpuCopyCmdAllocator->Reset();
    dGpuCopyCmdList->Reset(dGpuCopyCmdAllocator.Get(), nullptr);
    dGpuCopyCmdList->CopyResource(dGpuResourceMap["Copy"].Get(), dGpuResourceMap["Render"].Get());
    dGpuCopyCmdList->Close();

    // ======================================= Present =======================================
    iGpuCmdAllocator->Reset();
    iGpuCmdList->Reset(iGpuCmdAllocator.Get(), nullptr);
    iGpuCmdList->RSSetViewports(1, &ViewPort);
    iGpuCmdList->RSSetScissorRects(1, &ScissorRect);

    
    auto present2rtv = CD3DX12_RESOURCE_BARRIER::Transition(iGpuResourceMap[swapChainName].Get(), D3D12_RESOURCE_STATE_PRESENT,
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
    iGpuCmdList->ResourceBarrier(1, &present2rtv);

    auto iGpuRtvHandle = iGpuRtvHeap->GetCPUDescriptorHandleForHeapStart();
    iGpuRtvHandle.ptr += frameIndex * iGpuRtvDescriptorSize;
    iGpuCmdList->OMSetRenderTargets(1, &iGpuRtvHandle, FALSE, nullptr);
    iGpuCmdList->ClearRenderTargetView(iGpuRtvHandle, DirectX::Colors::LightYellow, 0, nullptr);
    iGpuCmdList->SetGraphicsRootSignature(iGpuRootSignature.Get());
    iGpuCmdList->SetPipelineState(iGpuPSO.Get());
    std::array<ID3D12DescriptorHeap *, 1> iGpuDescriptorHeaps = {iGpuSrvHeap.Get()};
    iGpuCmdList->SetDescriptorHeaps(static_cast<UINT>(iGpuDescriptorHeaps.size()), iGpuDescriptorHeaps.data());
    auto iGpuSrvHandle = iGpuSrvHeap->GetGPUDescriptorHandleForHeapStart();
    iGpuCmdList->SetGraphicsRootDescriptorTable(0, iGpuSrvHandle);

    iGpuCmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    iGpuCmdList->IASetVertexBuffers(0, 1, &iGpuVertexBufferView);
    iGpuCmdList->DrawInstanced(4, 1, 0, 0);

    auto rtv2present = CD3DX12_RESOURCE_BARRIER::Transition(iGpuResourceMap[swapChainName].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                            D3D12_RESOURCE_STATE_PRESENT);
    iGpuCmdList->ResourceBarrier(1, &rtv2present);
    iGpuCmdList->Close();

    // ======================================= Execute =======================================
    std::array<ID3D12CommandList *, 1>
        dGpuRenderCommandLists = {dGpuCmdList.Get()};
    dGpu3DQueue->ExecuteCommandLists(1, dGpuRenderCommandLists.data());
    dGpu3DQueue->Signal(dGpuFence.Get(), ++mFenceValue);

    dGpuCopyQueue->Wait(dGpuFence.Get(), mFenceValue);
    std::array<ID3D12CommandList *, 1>
        dGpuCopyCommandLists = {dGpuCopyCmdList.Get()};
    dGpuCopyQueue->ExecuteCommandLists(1, dGpuCopyCommandLists.data());
    dGpuCopyQueue->Signal(dGpuSharedFence.Get(), ++mFenceValue);

    iGpu3DQueue->Wait(iGpuSharedFence.Get(), mFenceValue);
    std::array<ID3D12CommandList *, 1>
        iGpuCommandLists = {iGpuCmdList.Get()};
    iGpu3DQueue->ExecuteCommandLists(1, iGpuCommandLists.data());
    iGpu3DQueue->Signal(iGpuFence.Get(), ++mFenceValue);

    SwapChain->Present(1, 0);
    FlushCommandQueue(iGpu3DQueue.Get(), iGpuFence.Get(), mFenceValue);
    // FlushCommandQueue(dGpu3DQueue.Get(),dGpuFence.Get(),mFenceValue);
}
LRESULT CALLBACK WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE: {
        // Save the DXSample* passed in to CreateWindow.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
        return 0;
    case WM_PAINT:
        OnRender();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}