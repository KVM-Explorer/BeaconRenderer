#pragma once
#include <stdafx.h>
#include "Texture.h"
#include "DescroptorHeap.h"
class TextureManager {
public:
    TextureManager(const TextureManager &) = default;
    TextureManager(TextureManager &&) = default;
    TextureManager &operator=(const TextureManager &) = default;
    TextureManager &operator=(TextureManager &&) = default;

    TextureManager(ID3D12Device *device, uint textureMaxNum);
    ~TextureManager();

    uint StoreTexture(Texture &texture);
    uint AddSrvDescriptor(uint resourceIndex, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN);
    uint AddUvaDescriptor(uint resourceIndex, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN); // TODO 修改StoreTexture
    Texture *GetTexture(uint index);
    void SetDescriptorName(std::string name, uint index);
    uint GetDescriptorName(std::string name);
    DescriptorHeap *GetTexture2DDescriptoHeap()
    {
        return mDescriptorHeap.get();
    }

private:
    uint mMaxNum;
    ID3D12Device *mDevice;
    uint mResourceIndex = 0;
    std::unique_ptr<DescriptorHeap> mDescriptorHeap;
    std::unordered_map<std::string, uint> mTextureMap;
    std::vector<Texture> mTexture2D;
    uint mDescriptorIndex = 0;
};