#pragma once
#include "../Graphics/Renderer.h"
#include "../World/World.h"
#include "CollisionSystem.h"
#include "Input.h"
#include "Player.h"
#include "Timer.h"
#include "Window.h"


class Application
{
public:
	struct Settings
	{
		int	  screenWidth  = 1920;
		int	  screenHeight = 1080;
		float fovDeg	   = 60;
		bool  fullscreen   = false;
		bool  vsync		   = true;
		// TODO: Add other settings here
	};

	Application();
	~Application();

	[[nodiscard]] bool Init();
	void			   Run();

	[[nodiscard]] Settings GetSettings() const { return settings_; }

private:
	void Update(float deltaTime);
	void OnWindowEvent(WindowEventBase& event);
	void HandleDebugInput();

	Window			window_;
	Renderer		renderer_;
	Input			input_;
	World			world_;
	Settings		settings_;
	Timer			timer_;
	Player			player_;
	CollisionSystem collisionSystem_;
};
