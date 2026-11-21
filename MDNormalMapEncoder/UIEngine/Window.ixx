module;
#include <windows.h>
#include <comutil.h>
#include <cassert>

export module UIEngine:Window;

import DebugUtils;
import std;

template <class T>
concept HasProcessWindowMessages = requires (T Reference, HWND _hWND, UINT _uMessage, WPARAM _wPARAM, LPARAM _lPARAM)
{
	{ Reference.ProcessWindowMessages(_hWND, _uMessage, _wPARAM, _lPARAM) } -> std::same_as<LRESULT>;
};

export template<class T>
class Window
{
	T* m_pWindowMessagesProcessor;
	size_t m_uWidth;
	size_t m_uHeight;
	std::wstring m_sTitle;
	LPCWSTR m_sApplicationName;

	HINSTANCE m_oModuleHandle;
	HWND m_oWindowHandle;

	static constexpr size_t WINDOW_OBJECT_OFFSET = 0;

	static DWORD GetWindowStyle(bool _bHasTitleBar, bool _bIsResizable)
	{
		DWORD uStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

		if (_bHasTitleBar)
			uStyle |= WS_OVERLAPPED | WS_CAPTION;
		else
			uStyle |= WS_POPUP;

		if (_bIsResizable)
			uStyle |= WS_SIZEBOX | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		else
			uStyle &= ~WS_SIZEBOX;

		return uStyle;
	}

	static LRESULT CALLBACK ProcessWindowMessages(HWND _hWND, UINT _uMessage, WPARAM _wPARAM, LPARAM _lPARAM)
	{
		Window* pWindow = reinterpret_cast<Window*>(GetWindowLongPtrW(_hWND, WINDOW_OBJECT_OFFSET));
		T* pWindowProcessor = pWindow == nullptr ? nullptr : pWindow->m_pWindowMessagesProcessor;
		return pWindowProcessor != nullptr ?
			pWindowProcessor->ProcessWindowMessages(_hWND, _uMessage, _wPARAM, _lPARAM):
			DefWindowProc(_hWND, _uMessage, _wPARAM, _lPARAM);
	}

public:
	Window(T* _pWindowMessagesProcessor, size_t _uWidth, size_t _uHeight, LPCWSTR _sTitle, LPCWSTR _sApplicationName, bool _bHasTitleBar, bool _bIsResizable)
		: m_pWindowMessagesProcessor(_pWindowMessagesProcessor), m_uWidth(_uWidth), m_uHeight(_uHeight), m_sTitle(_sTitle), m_sApplicationName(_sApplicationName)
	{
		static_assert(HasProcessWindowMessages<T>, "T must implement ProcessWindowMessages(HWND, UINT, WPARAM, LPARAM) -> LRESULT");

		// Get the instance of this application.
		m_oModuleHandle = GetModuleHandle(NULL);
		
		// Setup the window class with default settings.
		WNDCLASSEX oWindowClass = {
			.cbSize = sizeof(WNDCLASSEX),
			.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
			.lpfnWndProc = &ProcessWindowMessages,
			.cbClsExtra = 0,
			.cbWndExtra = sizeof(LONG_PTR),
			.hInstance = m_oModuleHandle,
			.hIcon = LoadIcon(NULL, IDI_WINLOGO),
			.hCursor = LoadCursor(NULL, IDC_ARROW),
			.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH),
			.lpszMenuName = nullptr,
			.lpszClassName = _sApplicationName,
			.hIconSm = oWindowClass.hIcon,
		};

		// Register the window class.
		RegisterClassEx(&oWindowClass);

		const DWORD uStyleEX = WS_EX_APPWINDOW;
		const DWORD uStyle = GetWindowStyle(_bHasTitleBar, _bIsResizable);

		// Compute the actual window size accounting for the title bar and borders.
		RECT oWindowRect{ 0L, 0L, static_cast<LONG>(_uWidth), static_cast<LONG>(_uHeight) };
		AdjustWindowRectEx(&oWindowRect, uStyle, false, uStyleEX);
		const int iWindowWidth = oWindowRect.right - oWindowRect.left;
		const int iWindowHeight = oWindowRect.bottom - oWindowRect.top;

		// Place the window in the middle of the screen.
		const int iWindowPositionX = (GetSystemMetrics(SM_CXSCREEN) - static_cast<int>(_uWidth)) / 2;
		const int iWindowPositionY = (GetSystemMetrics(SM_CYSCREEN) - static_cast<int>(_uHeight)) / 2;

		// Create the window with the screen settings and get the handle to it.
		m_oWindowHandle = CreateWindowEx(uStyleEX, _sApplicationName, _sTitle, uStyle,
			iWindowPositionX, iWindowPositionY, iWindowWidth, iWindowHeight, NULL, NULL, m_oModuleHandle, NULL);
		// Set the pointer to the message handler object.
		SetWindowLongPtrW(m_oWindowHandle, WINDOW_OBJECT_OFFSET, reinterpret_cast<LONG_PTR>(this));

		const bool bSuccess = m_oWindowHandle != NULL;
		if (!bSuccess)
			throw std::runtime_error(std::format("{}", DebugUtils::GetLastErrorMessage()));
		else
		{
			// Bring the window up on the screen and set it as main focus.
			ShowWindow(m_oWindowHandle, SW_SHOW);
			SetForegroundWindow(m_oWindowHandle);
			SetFocus(m_oWindowHandle);
		}
	}

	~Window()
	{
		// Remove the application instance.
		UnregisterClass(m_sApplicationName, m_oModuleHandle);
	}
};