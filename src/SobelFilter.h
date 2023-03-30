#include <stdafx.h>

class SobelFilter {
public:
    SobelFilter() = default;
    SobelFilter(const SobelFilter &) = default;
    SobelFilter(SobelFilter &&) = delete;
    SobelFilter &operator=(const SobelFilter &) = default;
    SobelFilter &operator=(SobelFilter &&) = delete;

    void Init(ID3D12Device *device);
    void Draw(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *input, ID3D12Resource *output);
    ID3D12Resource *Output();

private:
    void CompileShader();
    void CreateResource(ID3D12Device *device);
    void CreatePSO(ID3D12Device *device);
    void CreateRS(ID3D12Device *device);

    ComPtr<ID3D12RootSignature> mRS;
    ComPtr<ID3D12PipelineState> mPSO;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    uint mTextureSrvIndex;
    uint mTextureUvaIndex;
    uint mTextureResourceIndex;
};