#include "Application.h"

#include "Events/WindowEventFocusChange.h"
#include "Events/WindowEventResize.h"

Application::Application()
{
}

Application::~Application()
{
}

bool Application::Init()
{
	bool initSucceeded = window_.Init(settings_.screenWidth, settings_.screenHeight, settings_.fullscreen, L"Bloczki");
	window_.SetEventCallback(std::bind(&Application::OnWindowEvent, this, std::placeholders::_1));

	if (initSucceeded == false)
	{
		return false;
	}

	initSucceeded = renderer_.Initialize(window_.GetHandle(),
										 settings_.screenWidth,
										 settings_.screenHeight,
										 settings_.fullscreen,
										 true);

	if (initSucceeded == false)
	{
		return false;
	}

	initSucceeded = input_.Init(window_.GetInstance(), /**/
								window_.GetHandle(),
								settings_.screenWidth,
								settings_.screenHeight);

	if (initSucceeded == false)
	{
		return false;
	}

	const float aspectRatio = static_cast<float>(settings_.screenWidth) /**/
							/ static_cast<float>(settings_.screenHeight);

	world_.Initialize(renderer_.GetDevice().Get());
	collisionSystem_.Initialize(&world_);

	initSucceeded = player_.Initialize(&collisionSystem_,
									   &world_,
									   &input_,
									   settings_.fovDeg,
									   aspectRatio,
									   renderer_.GetNearZ(),
									   renderer_.GetFarZ());
	if (initSucceeded == false)
	{
		return false;
	}

	return true;
}

void Application::Run()
{
	MSG	 msg;
	bool done = false;

	// Initalize message structure
	ZeroMemory(&msg, sizeof(msg));

	world_.GenerateTestWorld();
	// Reset the timer, it might have some small amount of time from the initialization
	timer_.Reset();
	Timer perfCounter;
	bool  wireframeEnabled = true;
	// Loop until a quit message appears
	while (!done)
	{
		// Handle windows messages
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else // Per-frame stuff goes here
		{
			timer_.Tick();
			float deltaTime = timer_.GetDeltaTime();
			Update(deltaTime);

			if (input_.IsEscapePressed() == true)
			{
				done = true;
			}

			HandleDebugInput();

			perfCounter.Reset();
			world_.Update();
			perfCounter.TickUncapped();
			double worldPerf = perfCounter.GetDeltaTime();

			renderer_.BeginScene();
			renderer_.UpdateSkyConstants(world_.GetLightDirection(), world_.GetWorldTime());

			// World drawing goes here
			perfCounter.Reset();
			renderer_.RenderWorld(world_, player_.GetCamera());
			// renderer_.RenderSky();
			perfCounter.TickUncapped();
			float renderPerf = perfCounter.GetDeltaTime();

			// ALWAYS Draw this right before the end of drawing geometry, but before UI
			renderer_.DrawBlockOutline(player_.GetLastBlockRaycastResult());

			// UI Goes here
			renderer_.RenderUI(player_);
			renderer_.EndScene();
			perfCounter.Reset();

			DirectX::XMVECTOR cameraPos	   = player_.GetCamera().GetPosition();
			const float		  x			   = DirectX::XMVectorGetX(cameraPos);
			const float		  y			   = DirectX::XMVectorGetY(cameraPos);
			const float		  z			   = DirectX::XMVectorGetZ(cameraPos);
			Chunk*			  currentChunk = world_.GetChunkFromBlock(DirectX::XMFLOAT3{x, y, z});

			std::string windowTitle = ("Bloczki: "
									   + std::to_string(1.0 / deltaTime)
									   + " FPS"
									   + " | World took: "
									   + std::to_string(worldPerf * 1000.0)
									   + "ms"
									   + " | Renderer took: "
									   + std::to_string(renderPerf * 1000.0)
									   + "ms"
									   + " | World Time: "
									   + std::to_string(world_.GetWorldTime())
									   + " | Camera Position: "
									   + std::to_string(x)
									   + ", "
									   + std::to_string(y)
									   + ", "
									   + std::to_string(z));
			if (currentChunk != nullptr)
			{
				windowTitle += " | Current chunk: ";
				auto pos	 = currentChunk->GetChunkWorldPos();
				windowTitle += std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z);
			}
			if (player_.GetLastBlockRaycastResult().success)
			{
				windowTitle		   += " | Looking at block: ";
				auto raycastResult	= player_.GetLastBlockRaycastResult();
				int	 rayX			= raycastResult.blockPosition.x;
				int	 rayY			= raycastResult.blockPosition.y;
				int	 rayZ			= raycastResult.blockPosition.z;
				windowTitle		   += std::to_string(rayX) + ", " + std::to_string(rayY) + ", " + std::to_string(rayZ);

				windowTitle += " | BlockLight level: ";

				// Get the air block to get actual block light
				int normalX = raycastResult.faceNormal.x;
				int normalY = raycastResult.faceNormal.y;
				int normalZ = raycastResult.faceNormal.z;

				Block block	 = world_.GetBlock(DirectX::XMINT3{rayX + normalX, rayY + normalY, rayZ + normalZ});
				windowTitle += " Sky: "
							 + std::to_string(block.GetSkyLightLevel())
							 + " Block: "
							 + std::to_string(block.GetBlockLightLevel());
			}

			window_.SetWindowTitle(windowTitle);
		}
	}
}

void Application::Update(float deltaTime)
{
	// Anything that requires deltaTime will be called from here

	input_.Frame(deltaTime);
	player_.OnTick(deltaTime);
}

void Application::OnWindowEvent(WindowEventBase& event)
{
	switch (event.GetType())
	{
		case WindowEventType::FocusChange:
		{
			auto& focusEvent = dynamic_cast<WindowEventFocusChange&>(event);
			switch (focusEvent.GetFocusType())
			{
				case WindowEventFocusChange::FocusGained:
				{
					input_.OnGainFocus();
					break;
				}
				case WindowEventFocusChange::FocusLost:
				{
					input_.OnLoseFocus();
					break;
				}
			}
			break;
		}
		case WindowEventType::Resize:
		{
			auto& resizeEvent = dynamic_cast<WindowEventResize&>(event);
			float aspectRatio = static_cast<float>(resizeEvent.GetWidth()) //
							  / static_cast<float>(resizeEvent.GetHeight());
			Camera& camera = player_.GetCamera();
			camera.OnWindowResize(aspectRatio);

			renderer_.OnWindowResize(resizeEvent.GetWidth(), resizeEvent.GetHeight());
			break;
		}
	}
}
void Application::HandleDebugInput()
{
	static bool wireframeEnabled = true; // to pass "true" when we first press it
	if (input_.IsKeyPressed(DIK_F1))	 // reset debug mode
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::NONE);
	}
	if (input_.IsKeyPressed(DIK_F2)) // Albedo
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::Albedo);
	}
	if (input_.IsKeyPressed(DIK_F3)) // Normals
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::Normals);
	}
	if (input_.IsKeyPressed(DIK_F4)) // AO
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::AmbientOcclusion);
	}
	if (input_.IsKeyPressed(DIK_F5)) // Roughness
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::Roughness);
	}
	if (input_.IsKeyPressed(DIK_F6)) // Metallic
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::Metallic);
	}
	if (input_.IsKeyPressed(DIK_F7)) // Skylight
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::Skylight);
	}
	if (input_.IsKeyPressed(DIK_F8)) // BlockLight
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::BlockLight);
	}
	if (input_.IsKeyPressed(DIK_F9)) // Emissive
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::Emissive);
	}
	if (input_.IsKeyPressed(DIK_F10)) // Depth
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::Depth);
	}
	if (input_.IsKeyPressed(DIK_F11)) // Bloom
	{
		renderer_.SetDebugRenderMode(DebugRenderMode::Bloom);
	}

	static float pressTimer = 0.0f;
	if (input_.IsKeyPressed(DIK_F12)) // Wireframe
	{
		pressTimer += timer_.GetDeltaTime();
		if (pressTimer >= 0.5f)
		{
			renderer_.ToggleWireframe(wireframeEnabled);
			wireframeEnabled = !wireframeEnabled;
		}
	}
}
