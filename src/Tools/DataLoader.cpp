#include "DataLoader.h"
#include <iostream>
#include <fstream>
#include <MathHelper.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>


DataLoader::DataLoader(std::string path, std::string name) :
    mScenePath(path + "\\" + name),
    mRootPath(path),
    mName(name)
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
    // TODO ReDesign Root Transform
    std::vector<DirectX::XMFLOAT4X4> transforms;
    

    auto translation = DirectX::XMMatrixTranslation(-13.924f, -21.974f , 19.691f);
    // auto translation = DirectX::XMMatrixTranslation(-17.924f, -16.974f, 32.691f);
    // auto rotationX = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(-90));
    // auto rotationY = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(180));
    // auto rotationZ = DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(180));
    auto rotation = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90));
    auto scale = DirectX::XMMatrixScaling(0.1F, 0.1F, 0.1F);
    // auto root = scale * rotation * translation;

    if (mName == "lighthouse") {
        translation = DirectX::XMMatrixTranslation(-17.924f, -16.974f, -32.691f);
        scale = DirectX::XMMatrixScaling(0.016F, 0.016F, 0.016F);
        rotation = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(180));
    }
    
    for (const auto &item : mModel.Transforms) {
        auto itemTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&item));
        // itemTransform = itemTransform * MathHelper::SLHToRH();
        auto sceneTransform = translation * rotation * scale;
        auto result = itemTransform * sceneTransform;

        DirectX::XMFLOAT4X4 result4x4;
        DirectX::XMStoreFloat4x4(&result4x4, result);

        transforms.push_back(result4x4);
    }
    return transforms;

    // return mModel.Transforms;
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

Mesh DataLoader::ReadMesh(std::string path)
{
    Assimp::Importer importer;
    const uint flags = {aiProcess_ConvertToLeftHanded | aiProcess_Triangulate};
    auto *scene = importer.ReadFile(path.c_str(), flags);
    if (scene == nullptr) throw std::runtime_error("No Model Exist");
    auto const &mesh = scene->mMeshes;

    std::vector<ModelVertex> vertices;
    std::vector<uint16> indices;

    for (uint i = 0; i < scene->mNumMeshes; i++) {
        // Vertex
        for (uint j = 0; j < mesh[i]->mNumVertices; j++) {
            ModelVertex vertex{};
            vertex.Position = {mesh[i]->mVertices[j].x, mesh[i]->mVertices[j].y, mesh[i]->mVertices[j].z};
            vertex.Normal = {mesh[i]->mNormals[j].x, mesh[i]->mNormals[j].y, mesh[i]->mNormals[j].z};
            vertex.UV = {mesh[i]->mTextureCoords[0][j].x, mesh[i]->mTextureCoords[0][j].y};
            vertices.push_back(vertex);
        }
        // Index
        for (uint k = 0; k < mesh[i]->mNumFaces; k++) {
            auto face = mesh[i]->mFaces[k];
            for (uint j = 0; j < face.mNumIndices; j++) {
                indices.push_back(static_cast<uint16>(face.mIndices[j]));
                // TODO 数据量非常奇怪Vertex = Index，但是渲染结果正确
            }
        }
    }
    return {vertices, indices};
}

std::unordered_map<std::string, Mesh> DataLoader::GetMeshes()
{
    std::unordered_map<std::string, Mesh> ret;

    for (const auto &item : mModel.Models) {
        std::string path = mRootPath.string() + "\\" + item.mesh;
        std::replace(path.begin(), path.end(), '/', '\\');
        auto mesh = ReadMesh(path);
        ret[item.mesh] = mesh;
    }

    return ret;
}

std::vector<Model> DataLoader::GetModels() const

{
    std::vector<Model> models{mModel.Models[0]};
    // TODO Remove Test Signle Model
    return mModel.Models;

    // return models;
}
