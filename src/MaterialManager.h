#pragma once
#include <stdafx.h>
#include <DataStruct.h>

class MaterialManager {
public:
    MaterialManager(const MaterialManager &) = default;
    MaterialManager(MaterialManager &&) = default;
    MaterialManager &operator=(const MaterialManager &) = default;
    MaterialManager &operator=(MaterialManager &&) = default;

    uint AddMaterial(Material material /*,std::string name = ""*/);
    void UpdateMaterial(Material material, uint index);
    void Init(ID3D12Device *device);
    // Material GetMaterial(std::string name);
    Material GetMaterial(uint index);
    ID3D12Resource *Resource();

private:
    ComPtr<ID3D12Resource> mResource;
    std::vector<Material> mMaterials;
    uint mTotalNum;
};
