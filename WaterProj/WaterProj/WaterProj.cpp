#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "Map.h"
#include "MapRenderer.h"

int main()
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

	MapParams params;
	params.randomize(seed);
	Map* currentMap = new Map(2000, 2000, params, seed);
	MapRenderer renderer(currentMap);
	renderer.render(window);

	float height = 0.2f;
	bool exit = false;
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
					seed = 0;
					system("cls");
					std::cout << "Input Seed (or leave blank for random): ";
					std::getline(std::cin, input);
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
					params.randomize(seed);
					currentMap = new Map(2000, 2000, params, seed);
					renderer.setMap(currentMap);
					renderer.render(window);
					heightMode = false;
					height = 0.2f;
				}
				else if (event.key.keysym.sym == SDLK_w)
				{
					renderer.transformCam(glm::vec2(speed, speed));
					heightMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
				}
				else if (event.key.keysym.sym == SDLK_a)
				{
					renderer.transformCam(glm::vec2(speed, -speed));
					heightMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
				}
				else if (event.key.keysym.sym == SDLK_s)
				{
					renderer.transformCam(glm::vec2(-speed, -speed));
					heightMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
				}
				else if (event.key.keysym.sym == SDLK_d)
				{
					renderer.transformCam(glm::vec2(-speed, speed));
					heightMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
				}
				else if (event.key.keysym.sym == SDLK_q)
				{
					renderer.zoomIn();
					heightMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
				}
				else if (event.key.keysym.sym == SDLK_e)
				{
					renderer.zoomOut();
					heightMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
				}
				else if (event.key.keysym.sym == SDLK_l)
				{
					currentMap->erodeAllByValue(0.5f);
					renderer.render(window);
				}
				else if (event.key.keysym.sym == SDLK_UP)
				{
					if (heightMode)
					{
						height = height + 0.2f;
						renderer.renderAtHeight(window, height);
					}
				}
				else if (event.key.keysym.sym == SDLK_DOWN)
				{
					if (heightMode) 
					{
						height = height - 0.2f;
						renderer.renderAtHeight(window, height);
					}
				}
				else if (event.key.keysym.sym == SDLK_h)
				{
					heightMode = !heightMode;
					heightMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
				}
				else if (event.key.keysym.sym == SDLK_ESCAPE)
					exit = true;
				break;
			default:
				break;
			}
		}
	}
}