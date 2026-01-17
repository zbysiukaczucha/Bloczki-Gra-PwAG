#include "Input.h"

#include <tuple>

Input::Input()
{
}

bool Input::Init(HINSTANCE hInstance, HWND handle, int windowWidth, int windowHeight)
{
	HRESULT result;

	windowWidth_  = windowWidth;
	windowHeight_ = windowHeight;

	mouseX_ = 0;
	mouseY_ = 0;

	result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, &directInput_, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	// Initialize the keyboard
	result = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	result = keyboard_->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(result))
	{
		return false;
	}

	result = keyboard_->SetCooperativeLevel(handle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
	{
		return false;
	}

	result = keyboard_->Acquire();
	if (FAILED(result))
	{
		return false;
	}

	// Initialize the mouse
	result = directInput_->CreateDevice(GUID_SysMouse, &mouse_, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	result = mouse_->SetDataFormat(&c_dfDIMouse);
	if (FAILED(result))
	{
		return false;
	}

	result = mouse_->SetCooperativeLevel(handle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
	{
		return false;
	}

	result = mouse_->Acquire();
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool Input::Frame(double deltaTime)
{
	bool result;

	result = ReadKeyboardState(deltaTime);
	if (result == false)
	{
		return false;
	}

	result = ReadMouseState(deltaTime);
	if (result == false)
	{
		return false;
	}

	ProcessInput();

	return true;
}

bool Input::IsEscapePressed() const
{
	return keyboardState_[DIK_ESCAPE].isDown;
}

std::pair<int, int> Input::GetMousePosition() const
{
	return {mouseX_, mouseY_};
}

std::tuple<int, int, int> Input::GetMouseDelta() const
{
	return {mouseState_.lX, mouseState_.lY, mouseState_.lZ};
}

bool Input::IsKeyPressed(std::uint8_t key) const
{
	return keyboardState_[key].isDown;
}

bool Input::IsKeyPressed(MouseButton button) const
{
	std::uint8_t idx = static_cast<std::uint8_t>(button);
	return mouseButtonState_[idx].isDown;
}

void Input::OnLoseFocus()
{
	keyboard_->Unacquire();
	mouse_->Unacquire();
}

void Input::OnGainFocus()
{
	keyboard_->Acquire();
	mouse_->Acquire();
}

bool Input::ReadKeyboardState(double deltaTime)
{
	static std::array<std::uint8_t, 256> kbState;

	HRESULT result = keyboard_->GetDeviceState(kbState.size(), kbState.data());
	if (FAILED(result))
	{
		// If the keyboard lost focus or was not acquired then try to get control back.
		if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
		{
			keyboard_->Acquire();
		}
		else
		{
			return false;
		}
	}

	// put the data into the actual keyboard state struct
	for (std::size_t i = 0; i < kbState.size(); ++i)
	{
		KeyState& state = keyboardState_[i];
		state.wasDown	= state.isDown;
		state.isDown	= kbState[i] & 0x80;


		// Falling edge
		if (state.wasDown && state.isDown == false)
		{
			state.timeHeld		   = 0.0;
			state.timeSinceRelease = 0.0;
		}

		if (state.isDown)
		{
			state.timeHeld += deltaTime;
		}
		else
		{
			state.timeSinceRelease += deltaTime;
		}
	}

	return true;
}

bool Input::ReadMouseState(double deltaTime)
{
	HRESULT result = mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState_);
	if (FAILED(result))
	{
		// If the mouse lost focus or was not acquired then try to get control back.
		if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
		{
			mouse_->Acquire();
		}
		else
		{
			return false;
		}
	}

	for (std::size_t i = 0; i < std::size(mouseState_.rgbButtons); ++i)
	{
		KeyState& state = mouseButtonState_[i];
		state.wasDown	= state.isDown;
		state.isDown	= mouseState_.rgbButtons[i] & 0x80;


		// Falling edge
		if (state.wasDown && state.isDown == false)
		{
			state.timeHeld		   = 0.0;
			state.timeSinceRelease = 0.0;
		}

		if (state.isDown)
		{
			state.timeHeld += deltaTime;
		}
		else
		{
			state.timeSinceRelease += deltaTime;
		}
	}

	return true;
}

void Input::ProcessInput()
{
	mouseX_ += mouseState_.lX;
	mouseY_ += mouseState_.lY;

	// Lock mouse to window bounds
	mouseX_ = (mouseX_ > windowWidth_) //
				? windowWidth_		   //
				: (mouseX_ < 0)		   //
					  ? 0			   //
					  : mouseX_;

	mouseY_ = (mouseY_ > windowHeight_) //
				? windowHeight_			//
				: (mouseY_ < 0)			//
					  ? 0				//
					  : mouseY_;
}
