#pragma once
#include <stdafx.h>
#include <Framework/ImguiManager.h>
#include "Tools/CPUTimer.h"
#include "Tools/D3D12GpuTimer.h"
#include <yaml-cpp/yaml.h>

struct GResource {
    inline static std::unique_ptr<ImguiManager> GUIManager = nullptr;
    inline static std::unique_ptr<CPUTimer> CPUTimerManager = nullptr;
    inline static std::unique_ptr<D3D12GpuTimer> GPUTimer = nullptr;
    inline static std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
    inline static std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;

    inline static uint Width = 0;
    inline static uint Height = 0;

    inline static void Desctory()
    {
        GPUTimer = nullptr;
        CPUTimerManager = nullptr;
    }
    inline static YAML::Node config;

    inline static void Init(std::string path)
    {
        try {
            GResource::config = YAML::LoadFile(path);

        } catch (std::exception &e) {
            throw std::runtime_error("Failed to load config file: " + path + "\n" + e.what());
        }
        GResource::Width = config["Application"]["Width"].as<uint>();
        GResource::Height = config["Application"]["Height"].as<uint>();
        GResource::GUIManager = std::make_unique<ImguiManager>(); // CreateWindows会调用一次IMGUI::IO 必须被初始化
        GResource::CPUTimerManager = std::make_unique<CPUTimer>();
        std::strcpy(GResource::GUIManager->State.CameraSpeed, "0.01");
    }
};
