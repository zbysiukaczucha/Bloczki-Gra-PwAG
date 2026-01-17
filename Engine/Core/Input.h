#pragma once
#include <array>
#include <dinput.h>
#include <utility>
#include <windows.h>
#include <wrl/client.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

struct KeyState
{
	bool   isDown			= false;
	bool   wasDown			= false;
	double timeHeld			= 0.0;
	double timeSinceRelease = 0.0;
};

enum class MouseButton : std::uint8_t
{
	LEFT  = 0,
	RIGHT = 1,
};

class Input
{
public:
	Input();
	bool Init(HINSTANCE hInstance, HWND handle, int windowWidth, int windowHeight);
	bool Frame(double deltaTime);

	[[nodiscard]] bool IsEscapePressed() const;
	[[nodiscard]] bool IsKeyPressed(std::uint8_t key) const;
	[[nodiscard]] bool IsKeyPressed(MouseButton button) const;

	void OnLoseFocus();
	void OnGainFocus();

private:
	bool ReadKeyboardState(double deltaTime);
	bool ReadMouseState(double deltaTime);
	void ProcessInput();

	Microsoft::WRL::ComPtr<IDirectInput8>		directInput_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;

	std::array<KeyState, 256> keyboardState_;
	std::array<KeyState, 8>	  mouseButtonState_;
	DIMOUSESTATE			  mouseState_;

	int windowWidth_;
	int windowHeight_;

	int mouseX_;
	int mouseY_;

public:
	// Getters
	[[nodiscard]] const KeyState& GetKeyState(std::uint8_t key) const { return keyboardState_[key]; }
	[[nodiscard]] const KeyState& GetKeyState(MouseButton button) const
	{
		return mouseButtonState_[static_cast<std::uint8_t>(button)];
	}
	[[nodiscard]] std::pair<int, int>		GetMousePosition() const;
	[[nodiscard]] std::tuple<int, int, int> GetMouseDelta() const;
};
