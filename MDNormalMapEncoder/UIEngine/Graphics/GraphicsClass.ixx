module;

#include <windows.h>

//#include <EventManager/EventListener.h>

export module GraphicsClass;

class cGraphicsClass
{
	size_t m_uWindowWidth;
	size_t m_uWindowHeight;

	//cEventListener<cGraphicsClass> m_oEventListener;

public:
	cGraphicsClass(unsigned int _uScreenWidth, unsigned int _uScreenHeight, HWND _HWND):
		m_uWindowWidth(_uScreenWidth),
		m_uWindowHeight(_uScreenHeight)
	{
		//m_oEventListener.RegisterEventHandler(this, &cGraphicsClass::OnWindowResized, eEventType::WINDOW_RESIZED);
	}

	~cGraphicsClass() = default;

	size_t GetWindowWidth() const { return m_uWindowWidth; };
	size_t GetWindowHeight() const { return m_uWindowHeight; };

	bool Frame() { return true; }

	//void OnWindowResized(const cResizeWindowEvent& _oEvent);

private:
	bool Render() { return true; }
};
