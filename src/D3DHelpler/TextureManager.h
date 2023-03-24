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

    uint StoreTexture(Texture &texture, DXGI_FORMAT format);
    Texture *GetTexture(uint index);
    DescriptorHeap *GetTexture2DDescriptoHeap()
    {
        return mDescriptorHeap.get();
    }

private:
    void CreateDescriptor(Texture &texture, uint index, DXGI_FORMAT format);

private:
    uint mMaxNum;
    ID3D12Device *mDevice;
    uint mTotalIndex = 0;
    std::unique_ptr<DescriptorHeap> mDescriptorHeap;
    std::vector<Texture> mTexture2Ds;
};