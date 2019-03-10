#include <stdio.h>
#include <stdlib.h>
#include "vulkanEngine.h"
#include <exception>
#include <assert.h>
#include <chrono>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

int main(void)
{
	bool sdlInited = false;
	try {
		// Initialize SDL
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			fprintf(stderr, "Error: Failed to initialize SDL : %s\n", SDL_GetError());
			return 1;
		}
		sdlInited = true;

		// Set the screen size to be one quarter of the display.
		// Note: I've noticed this doesn't report the correct screen resolution based on
		//	Windows' display scaliing. For instance, on a 4k display (3840x2160) with 150%
		//	scaling, the reported resolution is 1440p (2560x1440)
		SDL_DisplayMode sdlDisplayMode;
		if (SDL_GetDesktopDisplayMode(0, &sdlDisplayMode) < 0)
		{
			fprintf(stderr, "Error: Failed to get the current display mode : %s\n", SDL_GetError());
			SDL_Quit();
			return 1;
		}
		int screenWidth = sdlDisplayMode.w / 2;
		int screenHeight = sdlDisplayMode.h / 2;
		if (VERBOSE)
			printf("Using screen size: %d x %d\n", screenWidth, screenHeight);

		SDL_Window *sdlWindow = SDL_CreateWindow("LearningVulkanAgain",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			screenWidth, screenHeight,
			SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);

		// Initialize the engine
		VulkanEngine engine;

		auto startTime = std::chrono::high_resolution_clock::now();
		engine.init(sdlWindow, screenWidth, screenHeight);
		auto endTime = std::chrono::high_resolution_clock::now();

		printf("Time to initialize engine: %lf seconds\n", std::chrono::duration<double>(endTime - startTime).count());
	}
	catch (std::exception &e)
	{
		fprintf(stderr, "ERROR (%s:%u): Caught exception : %s\n", __FILE__, __LINE__, e.what());
		system("pause");
		if (sdlInited)
			SDL_Quit();
		return 1;
	}

	system("pause");

	return 0;
}