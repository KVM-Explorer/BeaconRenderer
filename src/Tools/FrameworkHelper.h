#pragma once

#include "stdafx.h"
#include "MathHelper.h"
inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<uint32_t>(hr));
    return std::string(s_str);
}

class HandleException : public std::runtime_error {
public:
    HandleException(HRESULT handle) :
        std::runtime_error(HrToString(handle)),
        mHandle(handle)
    {
    }

private:
    const HRESULT mHandle;
};

inline void ThrowIfFailed(HRESULT handler)
{
    if (FAILED(handler)) {
        throw HandleException(handler);
    }
}

std::wstring string2wstring(std::string str);
std::string wstring2string(std::wstring wstr);

inline UINT CalculateConstantBufferByteSize(UINT byteSize)
{
    return (byteSize + 255) & ~255;
}

inline void SetDXGIDebug(IDXGIObject *object, std::string name)
{
    object->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.data());
}

inline uint32_t UpperMemorySize(uint32_t size, uint32_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

namespace YAML{
    // 特化
    template<>
    struct convert<MathHelper::Vec3ui> {
        static Node encode(const MathHelper::Vec3ui& rhs){
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
        }

        static bool decode(const Node& node,MathHelper::Vec3ui& rhs){
            if(!node.IsSequence() || node.size() != 3){
                return false;
            }

            rhs.x = node[0].as<uint32_t>();
            rhs.y = node[1].as<uint32_t>();
            rhs.z = node[2].as<uint32_t>();
            return true;
        }
    }; 
} // namespace YAML