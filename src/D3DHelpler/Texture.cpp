#include "Texture.h"

Texture::Texture(ID3D12Device *device, ID3D12GraphicsCommandList *commandList,
                             std::wstring path, bool isCube)
{
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    std::unique_ptr<uint8_t[]> ddsData;
    DirectX::DDS_ALPHA_MODE alphaMode = DirectX::DDS_ALPHA_MODE_UNKNOWN;

    ThrowIfFailed(DirectX::LoadDDSTextureFromFile(device,
                                                  path.c_str(),
                                                  &mBuffer,
                                                  ddsData,
                                                  subresources,
                                                  SIZE_MAX,
                                                  &alphaMode,
                                                  &isCube));
    UINT64 uploadBufferSize = GetRequiredIntermediateSize(mBuffer.Get(),
                                                          0,
                                                          static_cast<UINT>(subresources.size()));
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploader)));

    UpdateSubresources(commandList, mBuffer.Get(),
                       mUploader.Get(),
                       0,
                       0,
                       static_cast<UINT>(subresources.size()),
                       subresources.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mBuffer.Get(),
                                                        D3D12_RESOURCE_STATE_COPY_DEST,
                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);
}