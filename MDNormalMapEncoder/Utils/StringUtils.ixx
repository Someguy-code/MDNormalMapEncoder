module;

#include <Windows.h>

export module StringUtils;

import std;

export struct StringUtils
{
    // Convert a wide Unicode string to an UTF8 string
    static std::string WideStringToUTF8(const std::wstring& _sWideString)
    {
        if (_sWideString.empty())
            return {};
        int iSizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &_sWideString[0], (int)_sWideString.size(), NULL, 0, NULL, NULL);
        std::string oString(iSizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, &_sWideString[0], (int)_sWideString.size(), &oString[0], iSizeNeeded, NULL, NULL);
        return oString;
    }

    // Convert an UTF8 string to a wide Unicode String
    static std::wstring UTF8ToWideString(const std::string& _sString)
    {
        if (_sString.empty())
            return {};
        int iSizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &_sString[0], (int)_sString.size(), NULL, 0);
        std::wstring sWideString(iSizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, &_sString[0], (int)_sString.size(), &sWideString[0], iSizeNeeded);
        return sWideString;
    }
};