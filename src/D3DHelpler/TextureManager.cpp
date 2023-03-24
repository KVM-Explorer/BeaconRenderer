#include "TextureManager.h"

TextureManager::TextureManager(ID3D12Device *device, uint textureMaxNum) :
    mMaxNum(textureMaxNum),
    mDevice(device)
{
    mDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, textureMaxNum, true);
}

uint TextureManager::StoreTexture(Texture &texture, DXGI_FORMAT format)
{
    if (mTotalIndex + 1 > mMaxNum) throw std::runtime_error("TextureManager::StoreTexture OverMaxNum");
    uint currentIndex = mTotalIndex;
    mTotalIndex++;

    CreateDescriptor(texture, currentIndex, format);
    mTexture2Ds.push_back(std::move(texture));

    return currentIndex;
}

void TextureManager::CreateDescriptor(Texture &texture, uint index, DXGI_FORMAT format)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = texture.GetDesc().MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Format = format;

    mDevice->CreateShaderResourceView(texture.Resource(), &srvDesc, mDescriptorHeap->CPUHandle(index));
}

Texture *TextureManager::GetTexture(uint index)
{
    return &mTexture2Ds[index];
}