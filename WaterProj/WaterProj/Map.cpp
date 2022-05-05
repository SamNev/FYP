#include "Map.h"

#include <glm.hpp>
#include <ext.hpp>
#include <iostream>
#include <sstream>

#include "MapRenderer.h"
#include "Node.h"
#include "PerlinNoise.h"
#include "Plant.h"
#include "Drop.h"

Map::Map(int width, int height, MapParams params, unsigned int seed)
{
	defineSoils();
	m_nodes = new Node[width * height];
	m_width = width;
	m_height = height;
	m_maxHeight = 0.0f;
	m_scale = params.scale;
	m_params = params;

	// seed based on time or whatever was given
	if(seed == 0)
		srand(time(NULL));
	else
		srand(seed);

	// a selection of varying perlin noise is needed to generate complex terrain
	PerlinNoise noises[8];
	for (int i = 0; i < 8; i++)
	{
		int generatedSeed = rand() % 99999;
		noises[i] = PerlinNoise(generatedSeed);
	}

	// Ensure our values are valid-  rarity must be a multiple of scale in these regards or we hit rounding errors.
	params.hillRarity -= (params.hillRarity % m_scale);
	params.mountainRarity -= (params.mountainRarity % m_scale);
	params.divetRarity -= (params.divetRarity % m_scale);

	float completion = 0.0f;

	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			float val = noises[NoiseType_BaseVariance].noise(x, y, params.noiseSampleHeight) * params.baseVariance * 10;
			val = 0;
			const float base = (noises[NoiseType_Lie].noise(x/(params.lieChangeRate / m_scale), y/(params.lieChangeRate / m_scale), params.noiseSampleHeight) * params.liePeak / m_scale) + (params.lieModif / m_scale);
			const float hill = getHillValue(&noises[NoiseType_Hill], x, y, params.hillHeight, params.hillRarity);
			const float div = getDivetValue(&noises[NoiseType_Divet], x, y, params.hillHeight * params.divetHillScalar, params.divetRarity);
			const float mount = getMountainValue(&noises[NoiseType_Mountain], x, y, params.mountainHeight, params.mountainRarity);
			float total = (base + val + hill + mount + div);

#ifdef FLOODTESTMAP
			// custom map
			total = (abs(x-500) + abs(y-500))/20.0f;
#endif

			const float sandThreshold = noises[NoiseType_Sand].noise(x, y, params.noiseSampleHeight) * params.peakSandHeight;

			if (total < sandThreshold)
			{
				// sand (1.5g/cm3)
				m_nodes[y * width + x].addMarker(glm::max(BEDROCK_SAFETY_LAYER, total), params.sandResistivity, false, params.sandColor, params.sandFertility, 1.0f, 0.0f, m_maxHeight);
			}
			else
			{
				// topsoil (2.3g/cm3)
				float topNoise = noises[NoiseType_Resistivity].noise(x / (params.resistivityChangeRate / m_scale), y / (params.resistivityChangeRate / m_scale), glm::max(BEDROCK_SAFETY_LAYER, total));
				float sandAmount = params.soilSandContent + topNoise * params.soilSandVariance;
				float clayAmount = params.soilClayContent + topNoise * params.soilSandContent;
				float resistivity = params.resistivityBase + topNoise * params.resistivityVariance;
				m_nodes[y * width + x].addMarker(glm::max(BEDROCK_SAFETY_LAYER, total), resistivity, false, glm::vec3(0.2f + topNoise * 0.4f, 0.3f, 0.0f), params.soilFertility, sandAmount, clayAmount, m_maxHeight);
			}
			// bedrock (7.5g/cm3)
			m_nodes[y * width + x].addMarker(BEDROCK_LAYER, params.bedrockResisitivity, true, glm::vec3(0.1f), 0.0f, 0.0f, 0.0f, m_maxHeight);
			// fill all nodes to a basic "sea level"
			m_nodes[y * width + x].setWaterHeight(params.seaLevel);
		}

		// display progress
		float prevCompletion = completion;
		completion = (x / (float)width) * 100.0f;
		if((int)completion % 10 < (int) prevCompletion % 10)
		{
			if (prevCompletion < 10.0f)
				std::cout << "Generating World Nodes: " << completion << "%";
			else
				std::cout << std::string(3, '\b') << completion << "%";
		}
	}
	std::cout << std::string(3, '\b') << "100 %";
	std::cout << std::endl;

	addRocksAndDirt(&noises[NoiseType_Resistivity], &noises[NoiseType_Rock]);
}

void Map::addRocksAndDirt(PerlinNoise* resistivityNoise, PerlinNoise* rockNoise)
{
	// F=pV so resistivity and resistivity are linearly related
	float completion = 0.0f;
	float inc = 1.0f / (float)m_params.generatedMapDensity;

	for (int x = 0; x < m_width; ++x)
	{
		for (int y = 0; y < m_height; ++y)
		{
			bool isRock = false;
			float maxHeightScaled = getHeightAt(x, y) / m_maxHeight;

			if (rand() % m_params.treeGenerationRarity == 0)
				trySpawnTree(glm::vec2(x, y));

			// peak heights can be springs, spawning water constantly
			if (maxHeightScaled >= m_params.springThreshold && rand() % m_params.springRarity == 0)
				addSpring(x, y);

			for (float currHeight = 0.0f; currHeight < maxHeightScaled; currHeight += inc)
			{
				const float scaledDensHeight = currHeight * m_params.rockVerticalScaling;
				if (!isRock)
				{
					// rock resistiveForce (3.8-4.2g/cm3)
					float currVal = rockNoise->noise(x / (m_params.rockRarity / m_scale), y / (m_params.rockRarity / m_scale), scaledDensHeight);
					if (currVal > m_params.rockThreshold)
					{
						float resistivity = m_params.rockResistivityBase + (currVal - m_params.rockThreshold) * m_params.rockResistivityVariance;
						m_nodes[y * m_width + x].addMarker(currHeight * m_maxHeight, resistivity, true, glm::vec3(0.1f, 0.1f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.5f) * currVal, 0.0f, 0.0f, 0.0f, m_maxHeight);
						isRock = true;
					}
					else
					{
						// soil resistiveForce (2.3-2.6g/cm3, increasing with depth)
						float noise = resistivityNoise->noise(x / (m_params.resistivityChangeRate / m_scale), y / (m_params.resistivityChangeRate / m_scale), scaledDensHeight);
						float resistivity = m_params.resistivityBase + (1.0f - currHeight) + noise * m_params.resistivityVariance;
						float sandAmount = m_params.soilSandContent + noise * m_params.soilSandVariance;
						float clayAmount = m_params.soilClayContent + noise * m_params.soilSandContent;
						glm::vec3 col = glm::vec3(0.2f + noise * 0.2f, 0.3f, 0.0f);
						m_nodes[y * m_width + x].addMarker(currHeight * m_maxHeight, resistivity, false, col, m_params.soilFertility, sandAmount, clayAmount, m_maxHeight);
					}
				}
				else
				{
					// rock resistiveForce (3.8-4.2g/cm3)
					float currVal = rockNoise->noise(x / (m_params.rockRarity / m_scale), y / (m_params.rockRarity / m_scale), scaledDensHeight);
					if (currVal < m_params.rockThreshold)
					{
						float resistivity = m_params.rockResistivityBase + (currVal - m_params.rockThreshold) * m_params.rockResistivityVariance;
						m_nodes[y * m_width + x].addMarker(currHeight * m_maxHeight, resistivity, true, glm::vec3(0.1f, 0.1f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.5f) * currVal, 0.0f, 0.0f, 0.0f, m_maxHeight);
						isRock = false;
					}
				}
			}
		}

		float prevCompletion = completion;
		completion = (x / (float)m_width) * 100.0f;
		if ((int)completion % 10 < (int)prevCompletion % 10)
		{
			if (prevCompletion < 10.0f)
				std::cout << "Placing Rocks, Trees, and Dirt: " << completion << "%";
			else
				std::cout << std::string(3, '\b') << completion << "%";
		}
	}
	std::cout << std::string(3, '\b') << "100 %";
	std::cout << std::endl;
}

void Map::defineSoils()
{
	// All the soil types we need to know
	m_soilDefinitions.push_back(SoilDefinition("eapa", glm::vec2(0.24f, 0.29f), glm::vec2(0.29f, 0.34f), glm::vec2(0.2f, 0.75f), glm::vec2(0.0f, 2.3f)));
	m_soilDefinitions.push_back(SoilDefinition("attewan", glm::vec2(0.05f, 0.28f), glm::vec2(0.35f, 0.79f), glm::vec2(0.2f, 0.75f), glm::vec2(2.2f, 3.0f)));
	m_soilDefinitions.push_back(SoilDefinition("ethridge", glm::vec2(0.31f, 0.39f), glm::vec2(0.18f, 0.35f), glm::vec2(0.2f, 0.8f), glm::vec2(2.0f, 3.0f)));
	m_soilDefinitions.push_back(SoilDefinition("yamacall", glm::vec2(0.23f, 0.24f), glm::vec2(0.39f, 0.40f), glm::vec2(0.85f, 1.0f), glm::vec2(1.0f, 2.35f)));
	m_soilDefinitions.push_back(SoilDefinition("sand", glm::vec2(0.0f,0.05f), glm::vec2(0.8f,1.0f), glm::vec2(0.0f, 0.1f), glm::vec2(1.5f, 2.0f)));
	m_soilDefinitions.push_back(SoilDefinition("rock", glm::vec2(0.0f, 0.05f), glm::vec2(0.0f, 0.1f), glm::vec2(0.0f, 0.01f), glm::vec2(3.0f, 7.0f), true));
}

void Map::addSpring(int x, int y)
{
	m_springs.push_back(glm::vec2(x, y));
}

float Map::getHillValue(PerlinNoise* noise, int x, int y, float hillHeight, float rarity)
{
	glm::vec2 XY = calculateXYFromRarity(x, y, rarity);
	float hillVal = (noise->noise(XY.x, XY.y, m_params.noiseSampleHeight) - 0.1f) * glm::pow(noise->noise(XY.x, XY.y, m_params.noiseSampleHeight * 2.0f), m_params.hillVariancePower);
	return hillVal * hillHeight / m_scale;
}

float Map::getDivetValue(PerlinNoise* noise, int x, int y, float divetHeight, float rarity)
{
	glm::vec2 XY = calculateXYFromRarity(x, y, rarity);
	float divetVal = -(float)noise->noise(XY.x, XY.y, m_params.noiseSampleHeight);
	return divetVal * divetHeight / m_scale;
}

float Map::getMountainValue(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity)
{
	glm::vec2 XY = calculateXYFromRarity(x, y, rarity);
	float mountainVal = 0.0f;
	bool mountain = noise->noise(XY.x, XY.y, m_params.noiseSampleHeight) > m_params.mountainThreshold;
	if (mountain)
	{
		mountainVal += pow((noise->noise(XY.x, XY.y, m_params.noiseSampleHeight) - m_params.mountainThreshold) * sqrt(mountainHeight) * m_params.mountainConstantMultiplier / m_scale, 2);
	}
	return mountainVal;
}

glm::vec2 Map::calculateXYFromRarity(int x, int y, float rarity)
{
	float scalar = (rarity / m_scale);
	int lowX = x / scalar;
	int lowY = y / scalar;
	int highX = lowX + 1;
	int highY = lowY + 1;
	float xOffset = x % (int)scalar;
	float yOffset = y % (int)scalar;
	float xValue = lowX + (xOffset / scalar) * (highX - lowX);
	float yValue = lowY + (yOffset / scalar) * (highY - lowY);
	return glm::vec2(xValue, yValue);
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
	if (x < 0)
	{
		return getNodeAt(0, y);
	}
	if (y < 0)
	{
		return getNodeAt(x, 0);
	}

	return &m_nodes[y * m_width + x];
}

float Map::getHeightAt(int x, int y)
{
	return getNodeAt(x,y)->topHeight();
}

float Map::getHeightAt(int index)
{
	if (index >= (m_height * m_width))
	{
		return getHeightAt((m_height * m_width) - 1);
	}
	if (index < 0)
	{
		return getHeightAt(0);
	}

	return m_nodes[index].topHeight();
}

float Map::getDepthAt(int index)
{
	if (index >= (m_height * m_width))
	{
		return getDepthAt((m_height * m_width) - 1);
	}
	if (index < 0)
	{
		return getDepthAt(0);
	}

	return m_nodes[index].waterHeight(0.0f);
}

float Map::getDensityAt(int x, int y, float height)
{
	return getNodeAt(x, y)->getResistiveForceAtHeight(height);
}

// Debug function. Removes top layer of every node, to test resistiveForce values etc. without erosion
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

// Debug function. Erodes everything, to test erosion.
void Map::erodeAllByValue(float amount)
{
	for (int x = 0; x < m_width; ++x)
	{
		for (int y = 0; y < m_height; ++y)
		{
			getNodeAt(x, y)->erodeByValue(amount);
			getNodeAt(x, y)->setWaterHeight(0.0f);
		}
	}
}

Map::~Map()
{
	delete[m_width * m_height] m_nodes;
}

std::string Map::getSoilType(glm::vec2 pos)
{
	std::ostringstream oss;
	Node* node = getNodeAt(pos.x, pos.y);
	int bestIndex = -1;
	float bestCertainty = 0.0f;
	NodeMarker general = node->getDataAboveHeight(-1.9f, true);

	for (int i = 0; i < m_soilDefinitions.size(); i++)
	{
		float certainty = m_soilDefinitions[i].getCertainty(&general);
		if (certainty > bestCertainty)
		{
			bestCertainty = certainty;
			bestIndex = i;
		}
	}

	if (bestIndex != -1)
	{
		oss << m_soilDefinitions[bestIndex].name << " (" << bestCertainty << " % certainty)";
	}
	else
	{
		oss << "unknown";
	}

	NodeMarker loam = node->getDataAboveHeight(node->topHeight() - 1.0f);
	int mainIndex = bestIndex;
	bestIndex = -1;
	bestCertainty = 0.0f;

	for (int i = 0; i < m_soilDefinitions.size(); i++)
	{
		float certainty = m_soilDefinitions[i].getCertainty(&loam);
		if (certainty > bestCertainty)
		{
			bestCertainty = certainty;
			bestIndex = i;
		}
	}

	if (bestIndex != -1 && mainIndex != bestIndex)
	{
		oss << ", with a " << m_soilDefinitions[bestIndex].name << " loam (" << bestCertainty << "% certainty)" << std::endl;
	}

	return oss.str();
}

std::string Map::stats(glm::vec2 pos)
{
	std::ostringstream oss;
	glm::vec3 norm = normal(pos.y * m_width + pos.x);
	glm::vec3 col = getNodeAt(pos.x, pos.y)->topColor();
	glm::vec3 col2 = getNodeAt(pos.x, pos.y)->top()->color;
	oss << "Node data at pos " << pos.x << ", " << pos.y << ": \n Land height = " << getNodeAt(pos.x, pos.y)->topHeight() << std::endl;
	oss << " Pool = " << getNodeAt(pos.x, pos.y)->waterDepth() << std::endl << " Stream = " << getNodeAt(pos.x, pos.y)->getParticles() << std::endl << " Foliage = " << getNodeAt(pos.x, pos.y)->getFoliageDensity();
	oss << " Normal is " << norm.x << ", " << norm.y << ", " << norm.z << std::endl << " Color (with foliage and particles) is " << col.x << ", " << col.y << ", " << col.z << std::endl << " Node color is " << col2.x << ", " << col2.y << ", " << col2.z << std::endl;
	oss << " Soil type is " << getSoilType(pos) << std::endl;
	return oss.str();
}

void Map::erode(int cycles) {

	// track all particle movement
	bool* track = new bool[m_width * m_height];
	for (int i = 0; i < m_width * m_height; i++)
		track[i] = false;

	glm::vec2 dim = glm::vec2(m_width, m_height);
	int springIndex = 0;

	float completion = 0.0f;

	for (int i = 0; i < cycles; i++)
	{
		// spawn particle
		glm::vec2 newpos = glm::vec2(rand() % m_width, rand() % m_height);

		// spawn at spring if possible
		if (springIndex < m_springs.size())
		{
			newpos = m_springs.at(springIndex);
			springIndex++;
		}

		Drop drop(newpos);

		// if we've moved 1km, give up.
		int spill = 1000;

		while (drop.getVolume() > drop.getMinVolume() && spill != 0) {

			if (!drop.descend(normal((int)drop.getPosition().y * m_width + (int)drop.getPosition().x), m_nodes, track, dim, m_maxHeight) && drop.getVolume() > drop.getMinVolume())
			{
				if (!drop.flood(m_nodes, dim))
					break;
			}

			spill--;
		}

		if (spill == 0)
			drop.flood(m_nodes, dim);

		float prevCompletion = completion;
		completion = (i / (float)cycles) * 100.0f;
		if ((int)completion % 10 < (int)prevCompletion % 10)
		{
			if (prevCompletion < 10.0f)
				std::cout << "Running water simulation: " << completion << "%";
			else
				std::cout << std::string(3, '\b') << completion << "%";
		}
	}
	std::cout << std::string(3, '\b') << "100 %";
	std::cout << std::endl;

	int riverWidth = m_params.dropWidth * 2 + 1;
	for (int i = 0; i < m_width * m_height; i++)
	{
		if (track[i])
		{
			float h = m_nodes[i].topHeight() + 1.0f;

			for (int yOffset = 0; yOffset < riverWidth; yOffset++)
			{
				for (int xOffset = 0; xOffset < riverWidth; xOffset++)
				{
					getNodeAt(i % m_width - m_params.dropWidth, i / m_width - m_params.dropWidth)->setParticles(getNodeAt(i % m_width - m_params.dropWidth, i / m_width - m_params.dropWidth)->getParticles() + glm::min(1.0f, glm::max(0.0f, h - getNodeAt(i % m_width, i / m_width)->topHeight())));
				}
			}

			m_nodes[i].setParticles(glm::max(0.0f, m_nodes[i].getParticles() * (1.0f - (0.5f*m_params.waterEvaporationRate))));
		}
		else
		{
			m_nodes[i].setParticles(glm::max(0.0f, m_nodes[i].getParticles() * (1.0f - m_params.waterEvaporationRate)));
		}
	}

	delete[m_width * m_height] track;
}

void Map::grow()
{
	// spawn a tree randomly on the map (long-distance fertilization)
	// TODO: more of this? tweakable?
	int newTreePos = rand() % (m_width * m_height);
	int completion = 0;

	trySpawnTree(glm::vec2(newTreePos % m_width, newTreePos / m_width));

	for (int i = 0; i < m_width * m_height; i++)
	{
		// tree spawns a new tree
		if (m_nodes[i].getFoliageDensity() > 0.5f)
		{
			if (rand() % m_params.treeSpreadChance == 0)
			{
				glm::vec2 newPlantPos = glm::vec2(i % m_width, i / m_width) + glm::vec2(rand() % m_params.treeSpreadRadius - (m_params.treeSpreadRadius / 2), rand() % m_params.treeSpreadRadius - (m_params.treeSpreadRadius / 2));
				trySpawnTree(newPlantPos);
			}
			// TODO: lower base root value, re-add grow function. Additional fertility on river banks?

			// trees die in water & sometimes die randomly
			if (m_nodes[i].waterDepth() > 0.0 || m_nodes[i].getParticles() > m_params.treeParticleDeathThreshold || rand() % m_params.treeRandomDeathChance == 0)
			{
				Plant::root(m_nodes, glm::vec2(m_width, m_height), glm::vec2(i % m_width, i / m_width), -1.0f);
			}
		}

		float prevCompletion = completion;
		completion = (i / (float)(m_width * m_height)) * 100.0f;
		if ((int)completion % 10 < (int)prevCompletion % 10)
		{
			if (prevCompletion < 10.0f)
				std::cout << "Running foliage simulation: " << completion << "%";
			else
				std::cout << std::string(3, '\b') << completion << "%";
		}
	}
	std::cout << std::string(3, '\b') << "100 %";
	std::cout << std::endl;
};

bool Map::trySpawnTree(glm::vec2 pos)
{
	if (pos.x < 0 || pos.x >= m_width || pos.y < 0 || pos.y >= m_height)
		return false;

	int index = pos.y * m_width + pos.x;

	if (m_nodes[index].getFoliageDensity() >= m_params.foliageOverpopulationThreshold)
		return false;

	if (m_nodes[index].hasWater())
		return false;

	if(m_nodes[index].getParticles() > m_params.treeParticleDeathThreshold)
		return false;

	glm::vec3 norm = normal(index);
	if (abs(norm.z) < m_params.treeSlopeThreshold)
		return false;

	Plant::root(m_nodes, glm::vec2(m_width, m_height), pos, 0.5f);
	return true;
}