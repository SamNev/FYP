#include <chrono>
#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "Map.h"
#include "MapRenderer.h"

SDL_Window* makeSDLWindow()
{
	// Init SDL, create window and screen
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		throw std::exception("SDL Init failed!");
	}

	SDL_Window* window = SDL_CreateWindow("Water, water, everywhere!",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		900, 900,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	// V 4.5. Can be set to earlier versions if building for an older machine.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	if (!SDL_GL_CreateContext(window))
	{
		throw std::exception("OpenGL Context creation failed");
	}
	if (glewInit() != GLEW_OK)
	{
		throw std::exception("GlewInit failed");
	}

	return window;
}

unsigned int getSeed()
{
	system("cls");
	std::cout << "Input Seed (or leave blank for random): ";
	std::string input;
	std::getline(std::cin, input);
	unsigned int seed = 0;
	if (!input.empty())
	{
		seed = atoi(input.c_str());
	}
	else
	{
		srand(time(NULL));
		seed = rand();
		std::cout << "Seed is " << seed << std::endl;
	}

	return seed;
}

int main()
{
	SDL_Window* window = makeSDLWindow();
	unsigned int seed = getSeed();

	MapParams params;
	params.randomize(seed);
	Map* currentMap = new Map(1000, 1000, params, seed);
	MapRenderer renderer(currentMap);
	renderer.render(window);

	float height = 0.2f;
	bool exit = false;
	bool erodeMe = false;
	bool heightMode = false;
	while (!exit)
	{
		SDL_Event event;
		float speed = renderer.uncappedLodScaling() * 0.4f;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				exit = true;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_SPACE)
				{
					delete(currentMap);
					seed = getSeed();
					params.randomize(seed);
					currentMap = new Map(1000, 1000, params, seed);
					renderer.setMap(currentMap);
					heightMode = false;
					height = 0.2f;
				}
				else if (event.key.keysym.sym == SDLK_w)
				{
					renderer.transformCam(glm::vec2(speed, speed));
				}
				else if (event.key.keysym.sym == SDLK_a)
				{
					renderer.transformCam(glm::vec2(speed, -speed));
				}
				else if (event.key.keysym.sym == SDLK_s)
				{
					renderer.transformCam(glm::vec2(-speed, -speed));
				}
				else if (event.key.keysym.sym == SDLK_d)
				{
					renderer.transformCam(glm::vec2(-speed, speed));
				}
				else if (event.key.keysym.sym == SDLK_q)
				{
					renderer.zoomIn();
				}
				else if (event.key.keysym.sym == SDLK_e)
				{
					renderer.zoomOut();
				}
				else if (event.key.keysym.sym == SDLK_l)
				{
					currentMap->erodeAllByValue(0.5f);
				}
				else if (event.key.keysym.sym == SDLK_UP)
				{
					if (heightMode)
					{
						height = height + 0.2f;
					}
				}
				else if (event.key.keysym.sym == SDLK_DOWN)
				{
					if (heightMode) 
					{
						height = height - 0.2f;
					}
				}
				else if (event.key.keysym.sym == SDLK_h)
				{
					heightMode = !heightMode;
				}
				else if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					exit = true;
				}
				else if (event.key.keysym.sym == SDLK_p)
				{
					erodeMe = !erodeMe;
				}

				heightMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
				break;
			default:
				break;
			}
		}

		if (erodeMe)
		{
			auto start = std::chrono::system_clock::now();
			currentMap->erode(100);
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout << "Tick took " << elapsed_seconds.count() << "s" << std::endl;
			currentMap->grow();
			heightMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
		}
	}
}