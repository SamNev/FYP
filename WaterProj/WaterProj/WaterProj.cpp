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

	SDL_Window* window = SDL_CreateWindow("My Engine",
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

	MapParams params;
	//params.hillRarity = 2000;
	//params.hillHeight = 300;
	Map* currentMap = new Map(1000, 1000, params);
	MapRenderer renderer(currentMap);
	renderer.render(window);

	bool exit = false;
	while (!exit)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				exit = true;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_SPACE)
				{
					delete(currentMap);
					currentMap = new Map(1000, 1000, params);
					renderer.setMap(currentMap);
					renderer.render(window);
				}
				else if (event.key.keysym.sym == SDLK_w)
				{
					renderer.transformCam(glm::vec2(2.0f, 2.0f));
					renderer.render(window);
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