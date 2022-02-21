#include "Map.h"

#include <time.h>
#include <glm.hpp>
#include <ext.hpp>

#include "MapRenderer.h"
#include "Node.h"
#include "PerlinNoise.h"

Map::Map(int width, int height, MapParams params)
{
	m_nodes = new Node[width * height];
	m_width = width;
	m_height = height;
	m_scale = params.scale;

	srand(time(NULL));
	int seed = rand() % 9999;
	PerlinNoise perlin(seed);
	seed = rand() % 9999;
	PerlinNoise perlin2(seed);
	seed = rand() % 9999;
	PerlinNoise perlin3(seed);
	seed = rand() % 9999;
	PerlinNoise perlin4(seed);
	
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			const float val = perlin.noise(x, y, 0.5f) * params.baseVariance * glm::min(m_scale, 10.0f);
			const float base = (perlin4.noise(x/(params.lieChangeRate / m_scale), y/(params.lieChangeRate / m_scale), 0.5f) * params.liePeak / m_scale) + (params.lieModif / m_scale);
			const float hill = getHillValue(&perlin2, x, y, params.hillHeight, params.hillRarity);
			const float mount = getMountainValue(&perlin3, x, y, params.mountainHeight, params.mountainRarity);
			m_nodes[y * width + x].addMarker(base + val + hill + mount, 1.0f);
		}
	}
}

float Map::getHillValue(PerlinNoise* noise, int x, int y, float hillHeight, float rarity)
{
	float scalar = (rarity / m_scale);
	int lowX = x / scalar;
	int lowY = y / scalar;
	int highX = lowX + 1;
	int highY = lowY + 1;
	float xOffset = x % (int)scalar;
	float yOffset = y % (int)scalar;
	float hillX = lowX + (xOffset / scalar) * (highX - lowX);
	float hillY = lowY + (yOffset / scalar) * (highY - lowY);
	return (noise->noise(hillX, hillY, 0.5f) - 0.1f) * noise->noise(hillX, hillY, 0.0f) * hillHeight / m_scale;

}

float Map::getMountainValue(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity)
{
	float scalar = (rarity / m_scale);
	int lowX = x / scalar;
	int lowY = y / scalar;
	int highX = lowX + 1;
	int highY = lowY + 1;
	float xOffset = x % (int)scalar;
	float yOffset = y % (int)scalar;
	float mountX = lowX + (xOffset / scalar) * (highX - lowX);
	float mountY = lowY + (yOffset / scalar) * (highY - lowY);
	float mountainVal = 0.0f;
	bool mountain = noise->noise(mountX, mountY, 0.5f) > 0.85;
	if (mountain)
	{
		mountainVal += (noise->noise(mountX, mountY, 0.5f) - 0.85) * mountainHeight * 6.6f / m_scale;
	}
	return mountainVal;
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
}