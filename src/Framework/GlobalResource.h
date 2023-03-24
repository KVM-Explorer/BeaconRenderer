#pragma once
#include <stdafx.h>
#include <D3DHelpler/TextureManager.h>

struct GUIState {

};

class GResource {
public:
    inline static std::unique_ptr<TextureManager> TextureManager = nullptr;
    GUIState GUIState;
};

