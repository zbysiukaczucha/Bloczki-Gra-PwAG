#include "Window.h"

#include <iostream>
#include <ostream>

#include "Events/WindowEventFocusChange.h"
#include "Events/WindowEventResize.h"

Window* Window::WindowInstance = nullptr;

Window::Window()
{
	hInstance_			= nullptr;
	handle_				= nullptr;
	fullscreen_			= false;
	isMovingOrResizing_ = false;
	isMouseTrapped_		= false;
	screenWidth_		= 0;
	screenHeight_		= 0;
}

Window::~Window()
{
	WindowInstance = nullptr;
}

bool Window::Init(int windowWidth, int windowHeight, bool fullscreen, const std::wstring& windowName)
{
	if (hInstance_ == nullptr)
	{
		hInstance_ = GetModuleHandle(NULL);
	}

	windowName_ = windowName;
	fullscreen_ = fullscreen;

	screenWidth_  = GetSystemMetrics(SM_CXSCREEN);
	screenHeight_ = GetSystemMetrics(SM_CYSCREEN);

	if (fullscreen_)
	{
		windowWidth	 = screenWidth_;
		windowHeight = screenHeight_;
	}

	return InitializeWindows(windowWidth, windowHeight);
}

LRESULT Window::MessageHandler(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
			/*
			// Check if a key has been pressed on the keyboard.
			case WM_KEYDOWN:
			{
				// If a key is pressed, send it to the input object so it can record that state.
				return 0;
			}

			// Check if a key has been released on the keyboard.
			case WM_KEYUP:
			{
				// If a key is released then send it to the input object so it can unset the state for that key.
				return 0;
			}
			*/

		case WM_ENTERSIZEMOVE:
		{
			isMovingOrResizing_ = true;
			ToggleMouseTrap(false);
			return 0;
		}

		case WM_EXITSIZEMOVE:
		{
			isMovingOrResizing_ = false;
			return 0;
		}

		case WM_SETFOCUS:
		{
			// ShowCursor(false);
			SetCursor(nullptr);
			if (eventCallback_)
			{
				WindowEventFocusChange event(WindowEventFocusChange::FocusGained);
				eventCallback_(event);
			}

			if (isMovingOrResizing_ == false)
			{
				ToggleMouseTrap(true);
			}
			return 0;
		}
		case WM_KILLFOCUS:
		{
			// ShowCursor(true);
			if (eventCallback_)
			{
				WindowEventFocusChange event(WindowEventFocusChange::FocusLost);
				eventCallback_(event);
			}
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			ToggleMouseTrap(false);
			return 0;
		}
		case WM_SIZE:
		{
			// When the window is resized
			if (eventCallback_)
			{
				int				  width	 = LOWORD(lParam);
				int				  height = HIWORD(lParam);
				WindowEventResize event(width, height);
				eventCallback_(event);
			}
			return 0;
		}
		case WM_SETCURSOR:
		{
			if (LOWORD(lParam) == HTCLIENT)
			{
				if (GetFocus() == handle_)
				{
					SetCursor(nullptr);
					return 0;
				}
				else
				{
					SetCursor(LoadCursor(nullptr, IDC_ARROW));
					return 0;
				}
			}
		}

		default:
		{
			return DefWindowProc(handle, msg, wParam, lParam);
		}
	}
}

LRESULT Window::WndProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// Check if the window is being destroyed.
		case WM_DESTROY:
		// Check if the window is being closed.
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		// All other messages pass to the message handler in the system class.
		default:
		{
			return WindowInstance->MessageHandler(handle, msg, wParam, lParam);
		}
	}
	return 0;
}

void Window::SetWindowTitle(const std::string& title)
{
	SetWindowTextA(handle_, title.c_str());
}

bool Window::InitializeWindows(int windowWidth, int windowHeight)
{
	if (WindowInstance == nullptr)
	{
		WindowInstance = this;
	}
	WNDCLASSEX wc;
	wc.style	   = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = Window::WndProc;
	wc.cbClsExtra  = 0;
	wc.cbWndExtra  = 0;
	wc.hInstance   = hInstance_;
	wc.hIcon	   = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm	   = wc.hIcon;
	// wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hCursor		 = nullptr;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName	 = nullptr;
	wc.lpszClassName = windowName_.c_str();
	wc.cbSize		 = sizeof(WNDCLASSEX);

	if (!RegisterClassEx(&wc))
	{
		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_CLASS_ALREADY_EXISTS)
		{
			std::cerr << "RegisterClassEx failed: " << HRESULT_FROM_WIN32(dwError) << std::endl;
		}
		return false;
	}

	int posX, posY;
	if (fullscreen_)
	{
		// Set the screen size to max and 32 bit
		DEVMODE dmScreenSettings;
		ZeroMemory(&dmScreenSettings, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize		  = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth  = static_cast<DWORD>(screenWidth_);
		dmScreenSettings.dmPelsHeight = static_cast<DWORD>(screenHeight_);
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields	  = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// Set the position of the window to top left corner
		posX = posY = 0;
	}
	else
	{
		// If windowed, set the position of the window to the center of the screen
		posX = (screenWidth_ - windowWidth) / 2;
		posY = (screenHeight_ - windowHeight) / 2;
	}

	handle_ = CreateWindowEx(WS_EX_APPWINDOW,
							 windowName_.c_str(),
							 windowName_.c_str(),
							 WS_CLIPSIBLINGS
								 | WS_CLIPCHILDREN
								 | WS_POPUP
								 | WS_CAPTION
								 | WS_SYSMENU
								 | WS_MAXIMIZEBOX
								 | WS_MINIMIZEBOX
								 | WS_SIZEBOX,
							 posX,
							 posY,
							 windowWidth,
							 windowHeight,
							 nullptr,
							 nullptr,
							 hInstance_,
							 nullptr);

	if (handle_ == nullptr)
	{
		DWORD dwError = ::GetLastError();
		std::cerr << "CreateWindow failed: " << dwError << std::endl;
		return false;
	}

	// Show the window on the screen, set it as focus
	ShowWindow(handle_, SW_SHOW);
	SetForegroundWindow(handle_);
	SetFocus(handle_);

	// Hide mouse cursor
	// ShowCursor(false);

	return true;
}

void Window::ToggleMouseTrap(bool enable)
{
	if (enable)
	{
		RECT rect;
		GetClientRect(handle_, &rect);
		POINT topLeft	  = {rect.left, rect.top};
		POINT bottomRight = {rect.right, rect.bottom};

		MapWindowPoints(handle_, nullptr, &topLeft, 1);
		MapWindowPoints(handle_, nullptr, &bottomRight, 1);

		rect.left	= topLeft.x;
		rect.top	= topLeft.y;
		rect.right	= bottomRight.x;
		rect.bottom = bottomRight.y;

		isMouseTrapped_ = true;
		ClipCursor(&rect);
	}
	else
	{
		isMouseTrapped_ = false;
		ClipCursor(nullptr);
	}
}

void Window::ReleaseMouse()
{
	ClipCursor(nullptr);
}
