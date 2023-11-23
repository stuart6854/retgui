#include <retgui/retgui.hpp>

#include <SDL3/SDL.h>
// #include <SDL3/SDL_main.h>

#include <iostream>

int main()
{
	std::cout << "RetGui Testbed" << std::endl;

	const uint32_t WIN_WIDTH = 1080;
	const uint32_t WIN_HEIGHT = 720;
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return -1;
	}

	auto windowFlags = SDL_WindowFlags(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
	window = SDL_CreateWindow("RetGui Testbed", WIN_WIDTH, WIN_HEIGHT, windowFlags);
	if (window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return -1;
	}
	renderer = SDL_CreateRenderer(window, nullptr, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if (renderer == nullptr)
	{
		printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
		return -1;
	}

	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(window);

	// Main loop
	uint32_t lastFrameTime = SDL_GetPerformanceCounter();
	bool running = true;
	while (running)
	{
		uint32_t thisFrameTime = SDL_GetPerformanceCounter();
		float deltaTime = (thisFrameTime - lastFrameTime) / float(SDL_GetPerformanceFrequency());
		lastFrameTime = thisFrameTime;

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
				running = false;
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
				running = false;
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderPoint(renderer, 60, 60);

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_FRect rect;
		rect.x = 10.0f;
		rect.y = 10.0f;
		rect.w = 100.0f;
		rect.h = 100.0f;
		SDL_RenderRect(renderer, &rect);

		rect.x = WIN_WIDTH - rect.w - 10.0f;
		SDL_RenderFillRect(renderer, &rect);

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}