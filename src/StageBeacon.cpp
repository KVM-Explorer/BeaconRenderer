#include "StageBeacon.h"
#include "Framework/Application.h"
#include "GpuEntryLayout.h"

StageBeacon::StageBeacon(uint width, uint height, std::wstring title) :
    RendererBase(width, height, title),
    mViewPort(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
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

    CreateRtv();
    // Pass

    LoadAssets(); // Backend GPU
    CreateQuad(); // Display GPU

    // init Resource State (Resource init State is D3D12_RESOURCE_STATE_GENERIC_READ)
}

void StageBeacon::OnUpdate()
{
}

void StageBeacon::OnRender()
{
}

void StageBeacon::OnKeyDown(byte key)
{}

void StageBeacon::OnMouseDown(WPARAM btnState, int x, int y)
{}

void StageBeacon::OnDestory()
{

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
            mDisplayResource = std::make_unique<DisplayResource>(mFactory.Get(), adapter.Get());
        } else {
            auto outputStr = std::format(L"dGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
            OutputDebugStringW(outputStr.c_str());
            auto backendResource = std::make_unique<BackendResource>(mFactory.Get(), adapter.Get());
            mBackendResource.push_back(std::move(backendResource));
        }
    }
    if (mBackendResource.empty()) {
        throw std::runtime_error("No backend device found");
    }
    mDisplayResource->CreateSwapChain(mFactory.Get(), handle, GetWidth(), GetHeight(), mBackendResource.size());
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

void StageBeacon::CreateRtv()
{
}

void StageBeacon::LoadAssets()
{
}

void StageBeacon::CreateQuad()
{
}
