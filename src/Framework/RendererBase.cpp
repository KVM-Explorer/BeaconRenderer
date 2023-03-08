#include "RendererBase.h"

RendererBase::RendererBase(uint width, uint height, std::wstring title) :
    mWidth(width),
    mHeight(height),
    mTitle(title),
    MouseLastPosition({static_cast<LONG>(width/2),static_cast<LONG>(height/2)})
{
}

const wchar_t *RendererBase::GetTitle()
{
    return mTitle.c_str();
}

uint RendererBase::GetWidth()
{
    return mWidth;
}
uint RendererBase::GetHeight()
{
    return mHeight;
}