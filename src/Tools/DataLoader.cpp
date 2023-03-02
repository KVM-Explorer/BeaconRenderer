#include "DataLoader.h"
#include <iostream>
#include <fstream>
DataLoader::DataLoader(std::wstring path) :
    mScenePath(path)
{
    GetModelMetaFile();
    ReadSceneFromFile();
}

void DataLoader::ReadSceneFromFile()
{
    std::ifstream file(mMetaPath);
    std::stringstream reader;
    reader << file.rdbuf();

    std::string modelType = ReadType(reader);
    if (modelType == "blinn") {
        ReadLight(reader);
        ReadBlinnMaterials(reader);
        ReadTransforms(reader);
        ReadModels(reader);
    }else {
        throw std::exception("Unkonwn Model Type");
    }
}

void DataLoader::GetModelMetaFile()
{
    using namespace std::filesystem;
    if (!exists(mScenePath)) {
        throw std::runtime_error("Directory Not Exist");
    }

    directory_iterator filesList(mScenePath);
    for (const auto &file : filesList) {
        if (file.path().filename().extension() == ".scn") {
            mMetaPath = file.path();
            break;
        }
    }
}

std::string DataLoader::ReadType(std::stringstream &reader)
{
    std::string tmp;
    std::string type;
    reader >> tmp >> type;
    return type;
}

void DataLoader::ReadLight(std::stringstream &reader)
{
    std::string header;
    std::string tmp;
    float x, y, z;
    reader >> header;
    reader >> tmp >> x >> y >> z;
    mModel.LightInfo.background = {x, y, z, 1};
    reader >> tmp >> mModel.LightInfo.environment;
    reader >> tmp >> mModel.LightInfo.skybox;
    reader >> tmp >> mModel.LightInfo.shadow;
    reader >> tmp >> mModel.LightInfo.ambient;
    reader >> tmp >> mModel.LightInfo.punctual;
}

void DataLoader::ReadBlinnMaterials(std::stringstream &reader)
{
    std::string tmp;
    int num, index;
    float x, y, z, w;
    reader >> tmp >> num >> tmp;
    for (int i = 0; i < num; i++) {
        ModelMaterial material;
        reader >> tmp >> index >> tmp;
        reader >> tmp >> x >> y >> z >> w;
        material.basecolor = {x, y, z, w};
        reader >> tmp >> material.shiniess;
        reader >> tmp >> material.diffuse_map;
        reader >> tmp >> material.specular_map;
        reader >> tmp >> material.emission_map;
        reader >> tmp >> material.double_side;
        reader >> tmp >> material.enable_blend;
        reader >> tmp >> material.aplha_cutoff;
        mModel.Materials.emplace_back(material);
    }
}

void DataLoader::ReadTransforms(std::stringstream &reader)
{
    // TODO 注意数据的转置
    std::string tmp;
    int num, index;
    reader >> tmp >> num >> tmp;
    for (int i = 0; i < num; i++) {
        reader >> tmp >> index >> tmp;
        DirectX::XMFLOAT4X4 matrix;
        for (int j = 0; j < 4; j++) {
            reader >> matrix.m[j][0] >> matrix.m[j][1] >> matrix.m[j][2] >> matrix.m[j][3];
        }
        mModel.Transforms.emplace_back(matrix);
    }
}

void DataLoader::ReadModels(std::stringstream &reader)
{
    std::string tmp;
    int num, index;
    reader >> tmp >> num >> tmp;
    for (int i = 0; i < num; i++) {
        reader >> tmp >> index >> tmp;
        Model model;
        reader >> tmp >> model.mesh;
        reader >> tmp >> model.skeleton;
        reader >> tmp >> model.attached;
        reader >> tmp >> model.material;
        reader >> tmp >> model.transform;
        mModel.Models.emplace_back(model);
    }
}