#include "Map.h"

#include <time.h>
#include <glm.hpp>
#include <ext.hpp>

#include "MapRenderer.h"
#include "Node.h"
#include "PerlinNoise.h"

Map::Map(int width, int height)
{
	m_nodes = new Node[width * height];
	m_width = width;
	m_height = height;
	m_renderer = new MapRenderer(this);
	m_pointDist = 1.0f;

	srand(time(NULL));
	int seed = rand() % 9999;
	PerlinNoise perlin(seed);
	seed = rand() % 9999;
	PerlinNoise perlin2(seed);
	seed = rand() % 9999;
	PerlinNoise perlin3(seed);
	
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			const float val = perlin.noise(x, y, 0.5f) * 0.5;
			const float hill = getHillValue(&perlin2, x, y);
			const float mount = getMountainValue(&perlin3, x, y);
			m_nodes[y * width + x].addMarker(val + hill + mount, 1.0f);
		}
	}
}

float Map::getHillValue(PerlinNoise* noise, int x, int y)
{
	int lowX = x / 10;
	int lowY = y / 10;
	int highX = lowX + 1;
	int highY = lowY + 1;
	float xOffset = x % 10;
	float yOffset = y % 10;
	float hillX = lowX + (xOffset / 10.0) * (highX - lowX);
	float hillY = lowY + (yOffset / 10.0) * (highY - lowY);
	return noise->noise(hillX, hillY, 0.5f) * 4.0;

}

float Map::getMountainValue(PerlinNoise* noise, int x, int y)
{
	int lowX = x / 50;
	int lowY = y / 50;
	int highX = lowX + 1;
	int highY = lowY + 1;
	float xOffset = x % 50;
	float yOffset = y % 50;
	float mountX = lowX + (xOffset / 50.0) * (highX - lowX);
	float mountY = lowY + (yOffset / 50.0) * (highY - lowY);
	float mountainVal = 0.0f;
	bool mountain = noise->noise(mountX, mountY, 0.5f) > 0.9;
	if (mountain)
	{
		mountainVal += (noise->noise(mountX, mountY, 0.5f) - 0.9) * 100.0;
	}
	return mountainVal;
}

void Map::render(SDL_Window* window)
{
	m_renderer->render(window);
}

Node* Map::getNodeAt(int x, int y)
{
	if (y >= m_height)
	{
		return getNodeAt(x, fmin(y, m_height - 1));
	}
	if (x >= m_width)
	{
		return getNodeAt(fmin(x, m_width - 1), y);
	}

	return &m_nodes[y * m_width + x];
}

float Map::getHeightAt(int x, int y)
{
	if (y >= m_height)
	{
		return getHeightAt(x, fmin(y, m_height - 1));
	}
	if (x >= m_width)
	{
		return getHeightAt(fmin(x, m_width - 1), y);
	}
	if (x < 0)
	{
		return getHeightAt(0, y);
	}
	if (y < 0)
	{
		return getHeightAt(x, 0);
	}

	return m_nodes[y * m_width + x].top()->height;
}

Map::~Map()
{
	delete[m_width * m_height] m_nodes;
	delete(m_renderer);
}