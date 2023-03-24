#pragma once
#include <stdafx.h>
#include <D3DHelpler/TextureManager.h>
#include <Framework/ImguiManager.h>



class GResource {
public:
    inline static std::unique_ptr<TextureManager> TextureManager = nullptr;
    inline static std::unique_ptr<ImguiManager> GUIManager = nullptr;
};

