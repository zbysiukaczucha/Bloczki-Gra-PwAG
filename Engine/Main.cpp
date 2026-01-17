#include "Core/Application.h"

#ifdef _DEBUG
#include <cstdio>
#endif


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	bool createConsole = true;
	if (createConsole && AllocConsole())
	{
		FILE* fp;
		auto  error = freopen_s(&fp, "CONOUT$", "w", stdout); // Redirect stdout to console
		error		= freopen_s(&fp, "CONOUT$", "w", stderr); // Redirect stderr to console
		error		= freopen_s(&fp, "CONIN$", "r", stdin);	  // Redirect stdin to console

		printf("Debug Console Initialized.\n");
	}

	Application application;
	bool		result;

	// Initialize and run the system object.
	result = application.Init();
	if (result)
	{
		application.Run();
	}

	return 0;
}
