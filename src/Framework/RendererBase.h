#pragma once
#include <stdafx.h>

class RendererBase {
public:
    RendererBase(uint width,
                 uint height,
                 std::wstring title);
    RendererBase(const RendererBase &) = delete;
    RendererBase(RendererBase &&) = default;
    RendererBase &operator=(const RendererBase &) = delete;
    RendererBase &operator=(RendererBase &&) = default;

    virtual void OnInit() = 0;
    virtual void OnKeyDown(byte key){};
    virtual void OnMouseDown(){};
    virtual void OnDestory() = 0;

    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;

    uint GetWidth();
    uint GetHeight();
    const wchar_t *GetTitle();

protected:
    uint mWidth;
    uint mHeight;

private:
    std::wstring mTitle;
};