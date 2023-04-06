#include <stdafx.h>

class PassBase {
    explicit PassBase(uint targetNum);
    void SetTarget(std::vector<ID3D12Resource *> targets, ID3D12PipelineState *pso);
    void BeginPass(ID3D12GraphicsCommandList *cmdList);
    void ExecutePass(ID3D12GraphicsCommandList *cmdList);
    void EndPass(ID3D12GraphicsCommandList *cmdList);
    std::vector<ID3D12Resource *> Output();

private:
    uint mTargetNUm;
    std::vector<ID3D12Resource *> mTargets;
};