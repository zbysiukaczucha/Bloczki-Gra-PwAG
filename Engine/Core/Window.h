#pragma once

#define WIN32_LEAN_AND_MEAN

#include <functional>
#include <string>
#include <windows.h>

#include "Events/WindowEventBase.h"


class Window
{
public:
	Window();
	~Window();

	bool					Init(int windowWidth, int windowHeight, bool fullscreen, const std::wstring& windowName);
	LRESULT CALLBACK		MessageHandler(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam);
	void					SetWindowTitle(const std::string& title);

private:
	bool InitializeWindows(int windowWidth, int windowHeight);
	void ToggleMouseTrap(bool enable);
	void ReleaseMouse();

	static Window* WindowInstance;

	std::wstring windowName_;
	HINSTANCE	 hInstance_;
	HWND		 handle_;
	bool		 fullscreen_;
	int			 screenWidth_;
	int			 screenHeight_;
	int			 windowWidth_;
	int			 windowHeight_;
	bool		 isMovingOrResizing_;
	bool		 isMouseTrapped_;

	std::function<void(WindowEventBase&)> eventCallback_;

public:
	// Getters
	[[nodiscard]] HINSTANCE GetInstance() const { return hInstance_; }
	[[nodiscard]] HWND		GetHandle() const { return handle_; }
	[[nodiscard]] bool		IsFullscreen() const { return fullscreen_; }
	[[nodiscard]] int		GetScreenWidth() const { return screenWidth_; }
	[[nodiscard]] int		GetScreenHeight() const { return screenHeight_; }
	[[nodiscard]] int		GetWindowWidth() const { return windowWidth_; }
	[[nodiscard]] int		GetWindowHeight() const { return windowHeight_; }
	// Setters
	void SetEventCallback(std::function<void(WindowEventBase&)> function) { eventCallback_ = function; }
};
