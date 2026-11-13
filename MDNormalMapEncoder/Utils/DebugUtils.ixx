module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <comutil.h>
#include <cstdarg>
#include <stdio.h>
#include <windows.h>

export module DebugUtils;

import std;
import StringUtils;

export struct DebugUtils
{
	static void ErrorMessage(const char* _sMessage, ...)
	{
		static constexpr size_t uMaxDebugMessageLength = 256;
		char sFinalMessage[uMaxDebugMessageLength];
		va_list args;
		va_start(args, _sMessage);
		vsprintf_s(sFinalMessage, sizeof(sFinalMessage), _sMessage, args);
		va_end(args);
		OutputDebugStringA(sFinalMessage);
		int iButtonID = MessageBoxA(nullptr, sFinalMessage, "ERROR!!!", MB_ICONERROR | MB_OK);
		DebugBreak();
	}

	static std::wstring GetLastErrorMessageW()
	{
		static constexpr size_t uMaxDebugMessageLength = 1024;
		wchar_t oMessage[uMaxDebugMessageLength];

		const DWORD uLastErrorCode = GetLastError();
		const bool bSuccess = FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			uLastErrorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			oMessage,
			0,
			NULL);

		return bSuccess ?
			oMessage :
			std::format(L"Failed to format error message for error code {}", uLastErrorCode);
	}

	static std::string GetLastErrorMessage()
	{
		return StringUtils::WideStringToUTF8(DebugUtils::GetLastErrorMessageW());
	}
};