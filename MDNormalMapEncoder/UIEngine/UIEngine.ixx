module;
#include <windows.h>

export module UIEngine;

import :Window;
import DebugUtils;
import StringUtils;
import std;

export class UIEngine
{
    Window<UIEngine> m_oWindow;

	WPARAM ProcessMessageLoop()
	{
        while (true)
        {
            MSG oMessage;
            switch (GetMessage(&oMessage, NULL, 0, 0))
            {
            case -1:
                throw std::runtime_error(std::format("GetMessage failed: {}", DebugUtils::GetLastErrorMessage()));
                break;
            case 0:
                //exit loop
                return oMessage.wParam;
                break;
            default:
                TranslateMessage(&oMessage);
                DispatchMessage(&oMessage);
                break;
            }
        }
        return 0;
	}

public:
	UIEngine() : m_oWindow(this, 640u, 480u, L"Megadrive normal map encoder", L"MDNormalMapEncoder", true, true)
	{
	}

	int Execute()
	{
        return static_cast<int>(ProcessMessageLoop());
	}

	LRESULT ProcessWindowMessages(HWND _hWND, UINT _uMessage, WPARAM _wPARAM, LPARAM _lPARAM)
	{
		LRESULT uResult = 0;
		switch (_uMessage)
		{
		case WM_CLOSE:
			DestroyWindow(_hWND);
			uResult = 0;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			uResult = 0;
			break;
		default:
			uResult = DefWindowProcA(_hWND, _uMessage, _wPARAM, _lPARAM);
			break;
		}
		return uResult;
	}
};