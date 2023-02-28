#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN// Exclude rarely-used stuff from Windows headers.
#endif
#include <Windows.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <cstdint>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <string>
#include <array>

using Microsoft::WRL::ComPtr;


using byte = uint8_t;
using uint16 = uint16_t;
using uint = uint32_t;
using uint64 = uint64_t;
using int32 = int32_t;


