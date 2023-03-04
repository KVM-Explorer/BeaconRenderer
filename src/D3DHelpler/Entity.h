
#pragma once
#include "DataStruct.h"
#include "MathHelper.h"



class Entity {
public:
    Entity(EntityType type);

public:
    uint EntityIndex;
    DirectX::XMFLOAT4X4 Transform = MathHelper::Identity4x4();
    uint MaterialIndex;
    MeshInfo MeshInfo;

private:
    EntityType mType;
};