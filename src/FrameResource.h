#include <stdafx.h>

class FrameResource {
public:
    FrameResource(const FrameResource &) = default;
    FrameResource &operator=(const FrameResource &) = default;
    FrameResource(FrameResource &&) = delete;
    FrameResource &operator=(FrameResource &&) = delete;

    void Execute(ID3D12CommandQueue *cmdQueue,ID3D12Resource *target);
    void Sync();


private:
    ComPtr<ID3D12GraphicsCommandList> mCmdList;
    ComPtr<ID3D12CommandAllocator> mCmdAllocator;
     uint64_t mFenceValue = 0;

};