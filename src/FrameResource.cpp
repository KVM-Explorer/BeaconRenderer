#include "FrameResource.h"

void FrameResource::Reset() const
{
    CmdAllocator->Reset();
    CmdList->Reset(CmdAllocator.Get(), nullptr);
}

void FrameResource::Sync() const
{
    if (Fence->GetCompletedValue() < FenceValue) {
        HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(Fence->SetEventOnCompletion(FenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);

        CloseHandle(fenceEvent);
    }
}

void FrameResource::Signal(ID3D12CommandQueue *queue)
{
    CmdList->Close();
    std::array<ID3D12CommandList *, 1> taskList = {CmdList.Get()};
    queue->ExecuteCommandLists(taskList.size(), taskList.data());
    queue->Signal(Fence.Get(), ++FenceValue);
}

void FrameResource::Release()
{
    CmdList = nullptr;
    CmdAllocator = nullptr;
    Fence = nullptr;
}