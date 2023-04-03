#pragma once
#include <stdafx.h>

class PSOManager {
public:
    PSOManager(const PSOManager &) = default;
    PSOManager(PSOManager &&) = default;
    PSOManager &operator=(const PSOManager &) = default;
    PSOManager &operator=(PSOManager &&) = default;

    void SetPipelineState(std::string name, ComPtr<ID3D12PipelineState> pipelineState);
    ID3D12PipelineState *GetPipelineState(std::string name);

private:
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSO;
};