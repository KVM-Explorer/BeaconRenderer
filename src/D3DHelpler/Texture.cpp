#include "Texture.h"

Texture::Texture(ID3D12Device *device, ID3D12GraphicsCommandList *commandList,
                 std::wstring path, bool isCube)
{
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    std::unique_ptr<uint8_t[]> ddsData;
    DirectX::DDS_ALPHA_MODE alphaMode = DirectX::DDS_ALPHA_MODE_UNKNOWN;

    ThrowIfFailed(DirectX::LoadDDSTextureFromFile(device,
                                                  path.c_str(),
                                                  &mTexture,
                                                  ddsData,
                                                  subresources,
                                                  SIZE_MAX,
                                                  &alphaMode,
                                                  &isCube));
    mTexture->GetDesc();

    UINT64 uploadBufferSize = GetRequiredIntermediateSize(mTexture.Get(),
                                                          0,
                                                          static_cast<UINT>(subresources.size()));
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploader)));

    UpdateSubresources(commandList, mTexture.Get(),
                       mUploader.Get(),
                       0,
                       0,
                       static_cast<UINT>(subresources.size()),
                       subresources.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mTexture.Get(),
                                                        D3D12_RESOURCE_STATE_COPY_DEST,
                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);
}

Texture::Texture(ID3D12Device *device,
                 DXGI_FORMAT format,
                 uint width, uint height,
                 D3D12_RESOURCE_FLAGS flags,
                 bool isDepthTexture,
                 D3D12_HEAP_FLAGS heapFlags,
                 D3D12_RESOURCE_STATES initState)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(format,
                                                                      width,
                                                                      height,
                                                                      1,
                                                                      1,
                                                                      1,
                                                                      0,
                                                                      flags);
    if (flags & D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER)
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    float clearColor[4] = {0, 0, 0, 1.0F};
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = format;
    if (!isDepthTexture) {
        clearValue.Color[0] = clearColor[0];
        clearValue.Color[1] = clearColor[1];
        clearValue.Color[2] = clearColor[2];
        clearValue.Color[3] = clearColor[3];
    } else {
        clearValue.DepthStencil.Depth = 1.0F;
        clearValue.DepthStencil.Stencil = 0.0F;
    }
    if ((flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
        ThrowIfFailed(device->CreateCommittedResource(&heapProps,
                                                      D3D12_HEAP_FLAG_NONE,
                                                      &resourceDesc,
                                                      initState,
                                                      &clearValue,
                                                      IID_PPV_ARGS(&mTexture)));
    } else {
        ThrowIfFailed(device->CreateCommittedResource(&heapProps,
                                                      heapFlags,
                                                      &resourceDesc,
                                                      initState,
                                                      nullptr,
                                                      IID_PPV_ARGS(&mTexture)));
 
    }
}

Texture::Texture(ID3D12Resource *resource, ID3D12Resource *uploader)
    : mTexture(resource), mUploader(uploader)
{
}