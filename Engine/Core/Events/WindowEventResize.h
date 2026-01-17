#pragma once
#include "WindowEventBase.h"

class WindowEventResize : public WindowEventBase
{
public:
	WindowEventResize() = delete;

	WindowEventResize(std::uint32_t width, std::uint32_t height) :
		WindowEventBase(),
		windowWidth_(width),
		windowHeight_(height)
	{
	}

	[[nodiscard]] std::uint32_t GetWidth() const { return windowWidth_; }
	[[nodiscard]] std::uint32_t GetHeight() const { return windowHeight_; }

	// Overrides
	[[nodiscard]] WindowEventType GetType() const override { return WindowEventType::Resize; }
	[[nodiscard]] std::string	  ToString() const override { return "WindowEventResize"; }

private:
	std::uint32_t windowWidth_, windowHeight_;
};
