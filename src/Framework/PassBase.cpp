#include "PassBase.h"

PassBase::PassBase(ID3D12PipelineState *pso,ID3D12RootSignature *rs) :
    mPSO(pso),
    mRS(rs)
{
}

PassBase::~PassBase()
{
    mPSO = nullptr;
}
