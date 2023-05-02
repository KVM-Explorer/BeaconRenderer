#include "StageBeacon.h"

StageBeacon::StageBeacon(uint width, uint height, std::wstring title) :
    RendererBase(width, height, title),
    mViewPort(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
{
}

StageBeacon::~StageBeacon()
{
}

void StageBeacon::OnInit()
{
}

void StageBeacon::OnUpdate()
{
}

void StageBeacon::OnRender()
{
}

void StageBeacon::OnKeyDown(byte key)
{}

void StageBeacon::OnMouseDown(WPARAM btnState, int x, int y)
{
}

void StageBeacon::OnDestory()
{
}

