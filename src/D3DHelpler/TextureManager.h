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

    uint StoreTexture(Texture &texture);
    Texture GetTexture(uint index);
    ID3D12DescriptorHeap *GetTexture2DDescriptoHeap()
    {
        return mDescriptorHeap->Resource();
    }

private:
    void CreateDescriptor(Texture &texture,uint index);

private:
    uint mMaxNum;
    ID3D12Device *mDevice;
    uint mTotalIndex = 0;
    std::unique_ptr<DescriptorHeap> mDescriptorHeap;
    std::vector<Texture> m2DTextures;
};