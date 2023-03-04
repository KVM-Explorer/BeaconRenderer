#include "DataLoader.h"
#include <iostream>
#include <fstream>
DataLoader::DataLoader(std::wstring path, std::wstring name) :
    mScenePath(path + L"\\" + name),
    mRootPath(path)
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
    } else {
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

std::vector<DirectX::XMFLOAT4X4> DataLoader::GetTransforms() const
{
    return mModel.Transforms;
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

ModelLight DataLoader::GetLight() const
{
    return mModel.LightInfo;
}

std::vector<ModelMaterial> DataLoader::GetMaterials() const
{
    return mModel.Materials;
}

Mesh DataLoader::ReadMesh(std::wstring path)
{
    std::ifstream file(path);
    std::stringstream reader;
    std::string line;
    std::string tmp;
    std::string type;
    std::vector<DirectX::XMFLOAT3> vertices;
    std::vector<DirectX::XMFLOAT2> uvs;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<uint16> indices;

    while (getline(file, line)) {
        reader.clear();
        reader << line;
        reader >> type;

        if (type == "v") {
            DirectX::XMFLOAT3 vertex;
            reader >> vertex.x >> vertex.y >> vertex.z;
            vertices.emplace_back(vertex);
        }
        if (type == "vt") {
            DirectX::XMFLOAT2 uv;
            reader >> uv.x >> uv.y;
            uvs.emplace_back(uv);
        }
        if (type == "vn") {
            DirectX::XMFLOAT3 normal;
            reader >> normal.x >> normal.y >> normal.z;
            normals.emplace_back(normal);
        }
        if (type == "f") {
            uint16 index1, index2, index3;
            reader >> index1 >> tmp >> index1 >> tmp >> index1;
            reader >> index2 >> tmp >> index2 >> tmp >> index2;
            reader >> index3 >> tmp >> index3 >> tmp >> index3;
            indices.push_back(index1);
            indices.push_back(index2);
            indices.push_back(index3);
        }
    }
    if (vertices.size() != uvs.size() || vertices.size() != normals.size() || normals.size() != uvs.size()) {
        throw std::exception("Model Data Not Normalize");
    }

    std::vector<ModelVertex> modelVertex;
    for (uint i = 0; i < vertices.size(); i++) {
        modelVertex.push_back({vertices[i],
                               uvs[i],
                               normals[i]});
    }

    return {modelVertex, indices};
}

std::unordered_map<std::string, Mesh> DataLoader::GetMeshes()
{
    std::unordered_map<std::string, Mesh> ret;

    for (const auto &item : mModel.Models) {
        std::wstring path = mRootPath.wstring() + L"\\" + string2wstring(item.mesh);
        std::replace(path.begin(), path.end(), '/', '\\');
        auto mesh = ReadMesh(path);
        ret[item.mesh] = mesh;
    }

    return ret;
}

std::vector<Model> DataLoader::GetModels() const

{
    return mModel.Models;
}