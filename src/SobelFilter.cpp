#include "SobelFilter.h"
#include "Tools/FrameworkHelper.h"
#include "Framework/GlobalResource.h"
#include "D3DHelpler/Texture.h"

void SobelFilter::Init(ID3D12Device *device)
{
    CompileShader();
    CreateResource(device);
    CreateRS(device);
    CreatePSO(device);
}
void SobelFilter::CompileShader()
{
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/PostProcess.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "SobelMain", "cs_5_1", compileFlags, 0, &mShaders["SobelCS"], &error));
    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
}
void SobelFilter::CreateResource(ID3D12Device *device)
{
    Texture texture(device,
                    DXGI_FORMAT_R8G8B8A8_UNORM,
                    GResource::Width, GResource::Height,
                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    uint textureIndex = GResource::TextureManager->StoreTexture(texture);
    mTextureResourceIndex = textureIndex;
    mTextureSrvIndex = GResource::TextureManager->AddSrvDescriptor(textureIndex, DXGI_FORMAT_R8G8B8A8_UNORM);
    mTextureUvaIndex = GResource::TextureManager->AddUvaDescriptor(textureIndex, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void SobelFilter::CreateRS(ID3D12Device *device)
{
    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    std::array<CD3DX12_ROOT_PARAMETER, 2> rootParams;
    rootParams.at(0).InitAsDescriptorTable(1, &srvTable);
    rootParams.at(1).InitAsDescriptorTable(1, &uavTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(rootParams.size(),
                                            rootParams.data(),
                                            0,

                                            nullptr,
                                            D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> result;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc,
                                              D3D_ROOT_SIGNATURE_VERSION_1_0,
                                              &result, &error));
    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
    ThrowIfFailed(device->CreateRootSignature(0,
                                              result->GetBufferPointer(),
                                              result->GetBufferSize(),
                                              IID_PPV_ARGS(&mRS)));
}

void SobelFilter::CreatePSO(ID3D12Device *device)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.CS = CD3DX12_SHADER_BYTECODE(mShaders["SobelCS"].Get());
    desc.pRootSignature = mRS.Get();
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&mPSO)));
}

void SobelFilter::Draw(ID3D12GraphicsCommandList *cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle)
{
    auto *srvUavCbvHeap = GResource::TextureManager->GetTexture2DDescriptoHeap();
    cmdList->SetComputeRootSignature(mRS.Get());
    cmdList->SetPipelineState(mPSO.Get());
    cmdList->SetComputeRootDescriptorTable(0, srvHandle);
    cmdList->SetComputeRootDescriptorTable(1, srvUavCbvHeap->GPUHandle(mTextureUvaIndex));

    // TODO update group allocation
    uint numGroupX = GResource::Width / 16;
    uint numGroupY = GResource::Height / 16;
    cmdList->Dispatch(numGroupX, numGroupY, 1);
}

ID3D12Resource *SobelFilter::OuptputResource()
{
    return GResource::TextureManager->GetTexture(mTextureResourceIndex)->Resource();
}