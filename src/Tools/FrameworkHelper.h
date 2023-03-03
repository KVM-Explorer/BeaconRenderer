#pragma once
#include <stdexcept>
#include <Windows.h>

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