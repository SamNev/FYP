#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>

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


}