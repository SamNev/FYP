#include "Map.h"
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

	PerlinNoise perlin;
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			m_nodes[y * width + x].addMarker(perlin.noise(x, y, 0.5f), 1.0f);
		}
	}
}

void Map::render()
{
	m_renderer->render();
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