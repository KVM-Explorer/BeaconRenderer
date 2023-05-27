#include "DataLoader.h"
#include <iostream>
#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

DataLoader::DataLoader(std::string path, std::string name) :
    mScenePath(path + "\\" + name),
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
    } else if (modelType == "pbrm") {
        ReadLight(reader);
        ReadBPRMMaterials(reader);
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

void DataLoader::ReadBPRMMaterials(std::stringstream &reader)
{
    std::string tmp;
    /*
            basecolor_factor: 1 1 1 1
            metalness_factor: 1
            roughness_factor: 0.0
            basecolor_map: null
            metalness_map: null
            roughness_map: null
            normal_map: null
            occlusion_map: null
            emission_map: null
            double_sided: off
            enable_blend: off
            alpha_cutoff: 0
    */
    int num, index;
    float x, y, z, w;
    reader >> tmp >> num >> tmp;
    for (int i = 0; i < num; i++) {
        ModelMaterial material;
        reader >> tmp >> index >> tmp;
        reader >> tmp >> x >> y >> z >> w; // basecolor_factor
        material.basecolor = {x, y, z, w};
        reader >> tmp >> material.shiniess;     // metalness_factor !
        reader >> tmp >> material.shiniess;     // roughness_factor !
        reader >> tmp >> material.diffuse_map;  // basecolor_map
        reader >> tmp >> material.specular_map; // metalness_map
        reader >> tmp >> material.emission_map; // roughness_map
        reader >> tmp >> material.emission_map; // normal_map
        reader >> tmp >> material.emission_map; // occlusion_map
        reader >> tmp >> material.emission_map; // emission_map
        reader >> tmp >> material.double_side;  // double_sided
        reader >> tmp >> material.enable_blend; // enable_blend
        reader >> tmp >> material.aplha_cutoff; // alpha_cutoff
        material.shiniess = 1 - material.shiniess;
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

std::vector<DirectX::XMFLOAT4X4> DataLoader::GetTransforms(uint repeat) const
{
    using DirectX::XMMatrixTranspose;
    // TODO ReDesign Root Transform
    std::vector<DirectX::XMFLOAT4X4> transforms;
    for (uint i = 0; i < repeat + 1; i++) {
        auto translation = DirectX::XMMatrixTranslation(-13.924f, -21.974f, 19.691f);
        // auto translation = DirectX::XMMatrixTranslation(-17.924f, -16.974f, 32.691f);
        // auto rotationX = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(-90));
        // auto rotationY = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(180));
        // auto rotationZ = DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(180));
        // auto rotation = rotationX * rotationY;
        // auto scale = DirectX::XMMatrixScaling(0.1F, 0.1F, 0.1F);
        // auto root = scale * rotation * translation;

        for (const auto &item : mModel.Transforms) {
            auto itemTransform = XMMatrixTranspose(DirectX::XMLoadFloat4x4(&item));
            auto origin = DirectX::XMVectorGetZ(itemTransform.r[3]);
            itemTransform.r[3] = DirectX::XMVectorSetZ(itemTransform.r[3], origin + i * 10);
            auto result = itemTransform;

            DirectX::XMFLOAT4X4 result4x4;
            DirectX::XMStoreFloat4x4(&result4x4, result);

            transforms.push_back(result4x4);
        }
        
    }
    return transforms;
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
