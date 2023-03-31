#include "TextureManager.h"

TextureManager::TextureManager(ID3D12Device *device, uint textureMaxNum) :
    mMaxNum(textureMaxNum),
    mDevice(device)
{
    mDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, textureMaxNum, true);
}

uint TextureManager::StoreTexture(Texture &texture)
{
    if (mResourceIndex + 1 > mMaxNum) throw std::runtime_error("TextureManager::StoreTexture OverMaxNum");
    uint currentIndex = mResourceIndex;
    mResourceIndex++;
    mTexture2D.push_back(std::move(texture));

    return currentIndex;
}

uint TextureManager::AddSrvDescriptor(uint resourceIndex, DXGI_FORMAT format)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mTexture2D[resourceIndex].GetDesc().MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Format = format;

    uint index = mDescriptorIndex;
    mDescriptorIndex++;
    mDevice->CreateShaderResourceView(mTexture2D[resourceIndex].Resource(), &srvDesc, mDescriptorHeap->CPUHandle(index));
    return index;
}

uint TextureManager::AddUvaDescriptor(uint resourceIndex, DXGI_FORMAT format)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = format;
    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;
    uint index = mDescriptorIndex;
    mDescriptorIndex++;
    mDevice->CreateUnorderedAccessView(mTexture2D[resourceIndex].Resource(), nullptr, &desc, mDescriptorHeap->CPUHandle(index));
    return index;
}

Texture *TextureManager::GetTexture(uint index)
{
    return &mTexture2D[index];
}

void TextureManager::Destory()
{
    mTexture2D.clear();
    mDevice = nullptr;
}
