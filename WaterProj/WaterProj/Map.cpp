#include "Map.h"

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
	m_maxHeight = 0.0f;
	m_scale = params.scale;

	srand(time(NULL));
	int seed = rand() % 99999;
	PerlinNoise baseVarianceNoise(seed);
	seed = rand() % 99999;
	PerlinNoise hillNoise(seed);
	seed = rand() % 99999;
	PerlinNoise divetNoise(seed);
	seed = rand() % 99999;
	PerlinNoise mountainNoise(seed);
	seed = rand() % 99999;
	PerlinNoise lieNoise(seed);
	seed = rand() % 99999;
	PerlinNoise densityNoise(seed);
	seed = rand() % 99999;
	PerlinNoise rockNoise(seed);

	// Ensure our values are valid- hill and mountain rarity must be a multiple of scale
	params.hillRarity -= (params.hillRarity % m_scale);
	params.mountainRarity -= (params.mountainRarity % m_scale);
	
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			const float val = baseVarianceNoise.noise(x, y, 0.5f) * params.baseVariance * 10;
			const float base = (lieNoise.noise(x/(params.lieChangeRate / m_scale), y/(params.lieChangeRate / m_scale), 0.5f) * params.liePeak / m_scale) + (params.lieModif / m_scale);
			const float hill = getHillValue(&hillNoise, x, y, params.hillHeight, params.hillRarity);
			const float div = getDivetValue(&divetNoise, x, y, params.hillHeight/20.0f, params.divetRarity);
			const float mount = getMountainValue(&mountainNoise, x, y, params.mountainHeight, params.mountainRarity);
			// topsoil (2.3g/cm3)
			m_nodes[y * width + x].addMarker(glm::max(-1.9f, base + val + hill + mount + div), 2.3f, false, glm::vec3(0.0f, 1.0f, 0.0f), m_maxHeight);
			// bedrock (7.0g/cm3)
			m_nodes[y * width + x].addMarker(-2.0f, 7.5f, true, glm::vec3(0.1f), m_maxHeight);
		}
	}

	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			bool isRock = false;
			float maxHeightScaled = getHeightAt(x, y) / m_maxHeight;
			for (float densHeight = 0.0f; densHeight < maxHeightScaled; densHeight += 0.1f)
			{
				const float scaledDensHeight = densHeight * params.rockVerticalScaling;
				if (!isRock)
				{
					// soil density (2.5-2.8g/cm3)
					float noise = densityNoise.noise(x / (params.densityChangeRate / m_scale), y / (params.densityChangeRate / m_scale), scaledDensHeight);
					float density = 2.5f + noise * params.densityVariance;
					glm::vec3 col = glm::vec3(0.2f + noise * 0.4f, 0.3f, 0.0f);
					m_nodes[y * width + x].addMarker(densHeight * m_maxHeight, density, false, col, m_maxHeight);

					// rock density (3.2-3.8g/cm3)
					float currVal = rockNoise.noise(x / (params.rockRarity / m_scale), y / (params.rockRarity / m_scale), scaledDensHeight);
					if (currVal > 0.6f)
					{
						float density = 3.2f + (currVal - 0.6f) * params.rockDensityVariance;
						m_nodes[y * width + x].addMarker(densHeight * m_maxHeight, density, true, glm::vec3(0.1f, 0.1f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.5f) * currVal, m_maxHeight);
						isRock = true;
					}
				}
				else 
				{
					float currVal = rockNoise.noise(x / (params.rockRarity / m_scale), y / (params.rockRarity / m_scale), scaledDensHeight);
					if (currVal < 0.6f)
					{
						m_nodes[y * width + x].addMarker(densHeight * m_maxHeight, 3.2f, true, glm::vec3(0.1f, 0.1f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.5f) * currVal, m_maxHeight);
						isRock = false;
					}
				}
			}
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
	float hillVal = (noise->noise(hillX, hillY, 0.5f) - 0.1f) * glm::pow(noise->noise(hillX, hillY, 1.0f), 0.5f);

	return hillVal * hillHeight / m_scale;
}

float Map::getDivetValue(PerlinNoise* noise, int x, int y, float divetHeight, float rarity)
{
	int divetScalar = (rarity / m_scale);
	int divetLowX = x / divetScalar;
	int divetLowY = y / divetScalar;
	int divetHighX = divetLowX + 1;
	int divetHighY = divetLowY + 1;
	float divetXOffset = x % (int)divetScalar;
	float divetYOffset = y % (int)divetScalar;
	float divetHillX = divetLowX + (divetXOffset / divetScalar) * (divetHighX - divetLowX);
	float divetHillY = divetLowY + (divetYOffset / divetScalar) * (divetHighY - divetLowY);
	float divetVal = -(float)noise->noise(divetHillX, divetHillY, 0.5f);

	return divetVal * divetHeight / m_scale;

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
		mountainVal += pow((noise->noise(mountX, mountY, 0.5f) - 0.85) * sqrt(mountainHeight) * 6.6f / m_scale, 2);
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

float Map::getDensityAt(int x, int y, float height)
{
	return getNodeAt(x, y)->getDensityAtHeight(height);
}

// Debug function. Removes top layer of every node, to test density values etc. without erosion
void Map::skimTop()
{
	for (int x = 0; x < m_width; ++x)
	{
		for (int y = 0; y < m_height; ++y)
		{
			getNodeAt(x, y)->skim();
		}
	}
}

Map::~Map()
{
	delete[m_width * m_height] m_nodes;
}