#pragma once
#include "WindowEventBase.h"

class WindowEventFocusChange : public WindowEventBase
{
public:
	enum FocusType : std::uint8_t
	{
		FocusGained,
		FocusLost
	};

	WindowEventFocusChange() = delete;
	WindowEventFocusChange(FocusType focusType) :
		WindowEventBase(),
		focusType_(focusType)
	{
	}

	[[nodiscard]] FocusType GetFocusType() const { return focusType_; }

	// Overrides
	[[nodiscard]] WindowEventType GetType() const override { return WindowEventType::FocusChange; }
	[[nodiscard]] std::string	  ToString() const override { return "WindowEventFocusChange"; }

private:
	FocusType focusType_;
};
