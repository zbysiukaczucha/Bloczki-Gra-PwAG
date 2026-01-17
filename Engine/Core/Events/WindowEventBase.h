#pragma once
#include <string>


#include "WindowEventType.h"

class WindowEventBase
{
public:
	virtual ~WindowEventBase()							  = default;
	[[nodiscard]] virtual WindowEventType GetType() const = 0;

	[[nodiscard]] virtual std::string ToString() const { return "WindowEventBase"; }

	virtual void MarkHandled() { handled = true; }

protected:
	bool handled = false;
};
