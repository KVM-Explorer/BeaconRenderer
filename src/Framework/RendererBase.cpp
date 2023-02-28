#include "RendererBase.h"

RendererBase::RendererBase(uint width, uint height, std::wstring title) :
    mWidth(width),
    mHeight(height),
    mTitle(title)
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