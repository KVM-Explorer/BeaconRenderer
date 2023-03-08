#pragma once
#include "DDSTextureLoader12.h"
#include "d3dx12.h"
#include "Tools/FrameworkHelper.h"
#include <string>

class Texture {
public:
    Texture(const Texture &) = delete;
    Texture(Texture &&) = default;
    Texture &operator=(const Texture &) = delete;
    Texture &operator=(Texture &&) = default;

    Texture(ID3D12Device *device, ID3D12GraphicsCommandList *commandList,
            std::wstring path, bool isCube = false);

    [[nodiscard]] ID3D12Resource *Resource() const
    {
        return mBuffer.Get();
    }
    [[nodiscard]] D3D12_RESOURCE_DESC GetDesc()
    {
        return mBuffer->GetDesc();
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploader;
};
