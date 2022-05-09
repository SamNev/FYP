#include <chrono>
#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "Map.h"
#include "MapRenderer.h"

SDL_Window* makeSDLWindow()
{
	// Init SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		throw std::exception("SDL Init failed!");
	}

	SDL_Window* window = SDL_CreateWindow("Water Simulation Project",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		900, 900,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
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
	params.loadFromFile();
	Map* currentMap = new Map(1000, 1000, params, seed);
	MapRenderer renderer(currentMap);
	renderer.render(window);

	float height = 0.2f;
	bool exit = false;
	bool erosionEnabled = false;
	bool heightDisplayMode = false;

	while (!exit)
	{
		SDL_Event event;
		// Scale speed based on current zoom
		float speed = renderer.uncappedLodScaling() * 0.4f;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				exit = true;
				break;
			case SDL_KEYDOWN:
				// Change map
				if (event.key.keysym.sym == SDLK_r)
				{
					delete(currentMap);
					seed = getSeed();
					params.loadFromFile();
					currentMap = new Map(1000, 1000, params, seed);
					renderer.setMap(currentMap);
					heightDisplayMode = false;
					height = 0.2f;
				}
				// Camera movement
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

				// Debug
				else if (event.key.keysym.sym == SDLK_KP_1)
				{
					currentMap->erodeAllByValue(0.5f);
				}

				// Height view mode
				else if (event.key.keysym.sym == SDLK_UP)
				{
					if (heightDisplayMode)
					{
						height = height + 0.2f;
					}
				}
				else if (event.key.keysym.sym == SDLK_DOWN)
				{
					if (heightDisplayMode) 
					{
						height = height - 0.2f;
					}
				}
				else if (event.key.keysym.sym == SDLK_KP_2)
				{
					heightDisplayMode = !heightDisplayMode;
				}

				// Quit
				else if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					exit = true;
				}
				// play/pause
				else if (event.key.keysym.sym == SDLK_p)
				{
					erosionEnabled = !erosionEnabled;
				}
				// Get node position
				else if (event.key.keysym.sym == SDLK_i)
				{
					std::cout << "Which node? (x,y)" << std::endl;
					std::string choice;
					std::getline(std::cin, choice);
					
					int choicePos = choice.find(',');
					try {
						int locationY = stoi(choice.substr(choicePos + 1));
						int locationX = stoi(choice.substr(0, choicePos));
						std::cout << currentMap->stats(glm::vec2(locationX, locationY));
					}
					catch (std::exception e)
					{
						std::cout << "invalid input" << std::endl;
					}
				}

				// Current node stats
				else if (event.key.keysym.sym == SDLK_KP_3)
				{
					std::cout << currentMap->stats(glm::vec2(renderer.getCamPos().x, renderer.getCamPos().z)) << std::endl;
				}
				// Current position
				else if (event.key.keysym.sym == SDLK_KP_4)
				{
					std::cout << "Current position = " << renderer.getCamPos().x << ", " << renderer.getCamPos().z << std::endl;
				}
				// Go to position
				else if (event.key.keysym.sym == SDLK_KP_5)
				{
					std::cout << "Which node? (x,y)" << std::endl;
					std::string choice;
					std::getline(std::cin, choice);

					int choicePos = choice.find(',');
					try {
						int locationY = stoi(choice.substr(choicePos + 1));
						int locationX = stoi(choice.substr(0, choicePos));
						renderer.setCamPos(glm::vec3(locationX, 10.0f, locationY));
					}
					catch (std::exception e)
					{
						std::cout << "invalid input" << std::endl;
					}
				}
				else if (event.key.keysym.sym == SDLK_KP_6)
				{
					currentMap->grow();
				}
				else if (event.key.keysym.sym == SDLK_KP_7)
				{
					currentMap->erode(100);
				}
				else if (event.key.keysym.sym == SDLK_KP_8)
				{
					std::cout << currentMap->getMapGeneralSoilType();
				}

				heightDisplayMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
				break;
			default:
				break;
			}
		}

		if (erosionEnabled)
		{
			auto start = std::chrono::system_clock::now();
			currentMap->erode(100);
			auto erodeEnd = std::chrono::system_clock::now();
			currentMap->grow();
			auto growEnd = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsedTime = growEnd - start;
			std::chrono::duration<double> erodeTime = erodeEnd - start;
			std::chrono::duration<double> growTime = growEnd - erodeEnd;
			std::cout << "Year " << currentMap->getAge() << ". Tick took " << elapsedTime.count() << "s. " << erodeTime.count() << "s was eroding, " << growTime.count()<< " was growing" << std::endl << std::endl;
			heightDisplayMode ? renderer.renderAtHeight(window, height) : renderer.render(window);
		}
	}
}