#pragma once
#include <stdafx.h>
#include <filesystem>
#include <DataStruct.h>
#include <unordered_map>
#include "MathHelper.h"

struct ModelLight {
    DirectX::XMFLOAT4 background;
    std::string environment;
    std::string skybox;
    std::string shadow;
    float ambient;
    float punctual;
};

struct ModelMaterial {
    DirectX::XMFLOAT4 basecolor;
    float shiniess;
    std::string diffuse_map;
    std::string specular_map;
    std::string emission_map;
    std::string double_side;
    std::string enable_blend;
    float aplha_cutoff;
};

struct Model {
    std::string mesh;
    std::string skeleton;
    int attached;
    int material;
    int transform;
};

struct ModelProps {
    enum class ColorType {
        BliningPhong,
        PBRM,
    };
    ModelLight LightInfo;
    std::vector<ModelMaterial> Materials;
    std::vector<DirectX::XMFLOAT4X4> Transforms;
    std::vector<Model> Models;
    ColorType ColorType;
};

class DataLoader {
public:
    DataLoader(std::string path, std::string name);

    [[nodiscard]] std::vector<DirectX::XMFLOAT4X4> GetTransforms(MathHelper::Vec3ui repeat = {1, 1, 1}) const;
    /*
    @brief: Generate transforms for sinle model to render multiple models
    */
    [[nodiscard]] std::vector<DirectX::XMFLOAT4X4> GenerateTransforms(MathHelper::Vec3ui repeat) const;
    [[nodiscard]] ModelLight GetLight() const;
    [[nodiscard]] std::vector<ModelMaterial> GetMaterials() const;
    std::unordered_map<std::string, Mesh> GetMeshes();
    [[nodiscard]] std::vector<Model> GetModels() const;

private:
    void ReadSceneFromFile();
    void ReadSingleModel();
    void GetModelMetaFile();

    std::string ReadType(std::stringstream &reader);
    void ReadLight(std::stringstream &reader);
    void ReadBlinnMaterials(std::stringstream &reader);
    void ReadBPRMMaterials(std::stringstream &reader);
    void ReadTransforms(std::stringstream &reader);
    void ReadModels(std::stringstream &reader);

    Mesh ReadMesh(std::string path);

private:
    std::filesystem::path mScenePath;
    std::filesystem::path mRootPath;
    std::filesystem::path mMetaPath;
    std::string mName;
    ModelProps mModel;
};