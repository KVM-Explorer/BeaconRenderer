#pragma once
#include <stdafx.h>
#include "ImguiManager.h"
class RendererBase {
public:
    RendererBase(uint width,
                 uint height,
                 std::wstring title);
    RendererBase(const RendererBase &) = delete;
    RendererBase(RendererBase &&) = default;
    RendererBase &operator=(const RendererBase &) = delete;
    RendererBase &operator=(RendererBase &&) = default;

    virtual void OnInit(std::unique_ptr<ImguiManager> &guiContext) = 0;
    virtual void OnKeyDown(byte key){};
    virtual void OnMouseDown(WPARAM btnState, int x, int y){};
    virtual void OnDestory() = 0;

    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;

    uint GetWidth();
    uint GetHeight();
    const wchar_t *GetTitle();

protected:
    uint mWidth;
    uint mHeight;

    POINT MouseLastPosition;

private:
    std::wstring mTitle;
};