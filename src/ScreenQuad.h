#include <stdafx.h>
#include "D3DHelpler/DefaultBuffer.h"
#include "DataStruct.h"

class ScreenQuad {
public:
    ScreenQuad() = default;
    ScreenQuad(const ScreenQuad &) = default;
    ScreenQuad &operator=(const ScreenQuad &) = default;
    ScreenQuad(ScreenQuad &&) = delete;
    ScreenQuad &operator=(ScreenQuad &&) = delete;

    void SetState(ID3D12GraphicsCommandList *cmdList, QuadShader shaderID);
    void Draw(ID3D12GraphicsCommandList *cmdList);
    void Init(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);

private:
    void CreateInputLayout();
    void CompileShaders();
    void CreateRootSignature(ID3D12Device *device);
    void CreatePSO(ID3D12Device *device);
    void CreateVertices(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
    std::unique_ptr<DefaultBuffer> mQuadVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mQuadVertexView;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayot;
    ComPtr<ID3D12RootSignature> mRootSignature;
    ComPtr<ID3D12PipelineState> mPSO;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
};