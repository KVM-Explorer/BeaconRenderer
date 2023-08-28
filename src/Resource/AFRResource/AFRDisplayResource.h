#pragma once
#include "AFRResourceBase.h"
class AFRDisplayResource : public AFRResourceBase {
public:
    AFRDisplayResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount, std::wstring name);
    ~AFRDisplayResource();

    std::vector<HANDLE> CreateRenderTargets(uint width, uint height, size_t backendCount);
    void CreateSwapChain(IDXGIFactory6 *factory, HWND handle, uint width, uint height, size_t backendCount);
    std::vector<HANDLE> CreateSharedFence(size_t backendCount);

    std::vector<HANDLE> CreateCopyHeap(uint width, uint height, size_t backendCount);
    void CreateScreenQuadView();

    StageFrameResource *GetSharedFrameResource(int backendIndex, int frameIndex)
    {
        return &mSharedFR[backendIndex][frameIndex];
    }

    std::unique_ptr<SingleQuadPass> mSingleQuadPass;
    
    ComPtr<IDXGISwapChain4> SwapChain;

    std::unique_ptr<UploadBuffer<ModelVertex>> mQuadVB;
    std::unique_ptr<UploadBuffer<uint>> mQuadIB;

    std::vector<std::vector<StageFrameResource>> mSharedFR; // Device Count, Frame Count

    D3D12_VERTEX_BUFFER_VIEW ScreenQuadVBView;
    D3D12_INDEX_BUFFER_VIEW ScreenQuadIBView;

private:
    std::vector<ComPtr<ID3D12Heap>> mCopyHeaps;
};