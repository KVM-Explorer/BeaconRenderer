#include "Beacon.h"

Beacon::Beacon(uint width, uint height, std::wstring title) :
    RendererBase(width, height, title),
    mViewPort(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))

{
}

void Beacon::OnInit()
{
}

void Beacon::OnRender()
{
}

void Beacon::OnUpdate()
{
}

void Beacon::OnDestory()
{
}

void Beacon::OnMouseDown()
{
}

void Beacon::OnKeyDown(byte key)
{
}