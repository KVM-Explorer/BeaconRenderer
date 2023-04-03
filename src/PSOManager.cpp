#include "PSOManager.h"

#include <utility>

void PSOManager::SetPipelineState(std::string name, ComPtr<ID3D12PipelineState> pipelineState)
{
    if (mPSO.contains(name)) throw std::runtime_error("Multi PSO with same name");
    mPSO[name] = pipelineState;
}

ID3D12PipelineState *PSOManager::GetPipelineState(std::string name)
{
    if (!mPSO.contains(name)) throw std::runtime_error("PSOManager no target PSO");
    return mPSO[name].Get();
}
