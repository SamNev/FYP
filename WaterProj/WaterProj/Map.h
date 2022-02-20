#pragma once

#include "Node.h"
#include "SDL2/SDL.h"

class MapRenderer;
class PerlinNoise;

class Map 
{
public:
	Map(int width, int height);
	~Map();

	void render(SDL_Window* window);
	int getWidth() { return m_width; }
	int getHeight() { return m_height; }
	Node* getNodeAt(int x, int y);
	float getHeightAt(int x, int y);
	float getHillValue(PerlinNoise* noise, int x, int y);
	float getMountainValue(PerlinNoise* noise, int x, int y);
protected:
	Node* m_nodes;
	MapRenderer* m_renderer;
	int m_width;
	int m_height;
	float m_pointDist;
};