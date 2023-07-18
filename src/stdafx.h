#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers.
#endif
#include <Windows.h>
#include <windowsx.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <cstdint>
#include <vector>
#include <queue>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgidebug.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <string>
#include <array>
#include <memory>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx12.h>
#include <imgui/imgui_impl_win32.h>
#include <chrono>
#include <unordered_map>
#include <ppl.h>
#include <yaml-cpp/yaml.h>
using Microsoft::WRL::ComPtr;

using byte = uint8_t;
using uint16 = uint16_t;
using uint = uint32_t;
using uint64 = uint64_t;
using int32 = int32_t;
