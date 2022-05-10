#include "Map.h"

#include <glm.hpp>
#include <ext.hpp>
#include <sstream>

#include "Drop.h"
#include "MapRenderer.h"
#include "Node.h"
#include "PerlinNoise.h"
#include "Plant.h"

///////////////////////////////////////////////////////////////////////////////// MapParams

void MapParams::loadFromFile()
{
	// Fetch executable name
	bool found = false;
	wchar_t strExePath[MAX_PATH];
	GetModuleFileName(NULL, strExePath, MAX_PATH);
	size_t origsize = wcslen(strExePath) + 1;
	size_t convertedChars = 0;
	const size_t newsize = origsize * 2;
	char* nstring = new char[newsize];
	wcstombs_s(&convertedChars, nstring, newsize, strExePath, _TRUNCATE);

	// FolderPath created from executable name
	std::string folderPath = std::string(nstring);
	std::string executableName = folderPath.substr(folderPath.find_last_of("\\"));
	executableName = executableName.substr(1, executableName.length() - 5);
	std::string fullpath;
	while (!found && folderPath.find_last_of("\\") != std::string::npos)
	{
		// Search upwards for folder matching exe name
		folderPath.erase(folderPath.find_last_of("\\"));
		fullpath = folderPath + "\\" + executableName + "\\" + "params.txt";
		std::ifstream file(fullpath);
		if (file.is_open())
		{
			// If file is found
			std::string currentLine;
			int parameterCount = 0;
			while (std::getline(file, currentLine))
			{
				try {
					int spaceIndex = currentLine.find(' ');
					std::string param = currentLine.substr(0, spaceIndex);
					// Search for float or int parameter
					std::map<std::string, float&>::iterator floatParamPos = floatPropertyMap.find(param);
					std::map<std::string, int&>::iterator intParamPos = intPropertyMap.find(param);
					if (floatParamPos != floatPropertyMap.end())
					{
						float val = stof(currentLine.substr(spaceIndex + 1));
						floatParamPos->second = val;
						parameterCount++;
					}
					else if (intParamPos != intPropertyMap.end())
					{
						int val = stoi(currentLine.substr(spaceIndex + 1));
						intParamPos->second = val;
						parameterCount++;
					}
					else
					{
						// This error message is never shown- caught below
						throw std::exception("didn't find property?");
					}
				}
				catch (std::exception e)
				{
					// If line is empty or is a comment, don't show error message
					if (currentLine.length() > 2)
					{
						if (currentLine.find("//") != 0)
						{
							std::cout << "Failed to parse parameter from line \"" << currentLine << "\"" << std::endl;
						}
					}
				}
			}
			std::cout << parameterCount << " parameters loaded from file." << std::endl;
			found = true;
			file.close();
		}
	}

	if (!found)
	{
		std::cout << "Failed to find params file, assuming default values." << std::endl;
	}
}

///////////////////////////////////////////////////////////////////////////////// Map

Map::Map(int width, int height, MapParams params, unsigned int seed)
{
	defineSoils();
	m_nodes = new Node[width * height];
	m_width = width;
	m_height = height;
	m_maxHeight = 0.0f;
	m_params = params;

	// Seed based on time or whatever was given
	if(seed == 0)
		srand(time(NULL));
	else
		srand(seed);

	// A selection of varying perlin noise is needed to generate complex terrain
	PerlinNoise noises[8];
	for (int i = 0; i < 8; i++)
	{
		int generatedSeed = rand() % 99999;
		noises[i] = PerlinNoise(generatedSeed);
	}

	// Ensure our values are valid-  rarity must be a multiple of scale in these parameters or we hit rounding errors.
	m_params.hillRarity -= (m_params.hillRarity % m_params.scale);
	m_params.mountainRarity -= (m_params.mountainRarity % m_params.scale);
	m_params.divetRarity -= (m_params.divetRarity % m_params.scale);

	float completion = 0.0f;

	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)
		{
			float val = noises[NoiseType_BaseVariance].noise(x, y, m_params.noiseSampleHeight) * m_params.baseVariance;
			const float base = (noises[NoiseType_Lie].noise(x/(m_params.lieChangeRate / m_params.scale), y/(m_params.lieChangeRate / m_params.scale), m_params.noiseSampleHeight) * m_params.liePeak / m_params.scale) + (m_params.lieModif / m_params.scale);
			const float hill = getHillValue(&noises[NoiseType_Hill], x, y, m_params.hillHeight, m_params.hillRarity);
			const float div = getDivetValue(&noises[NoiseType_Divet], x, y, m_params.hillHeight * m_params.divetHillScalar, m_params.divetRarity);
			const float mount = getMountainValue(&noises[NoiseType_Mountain], x, y, m_params.mountainHeight, m_params.mountainRarity);
			float total = (base + val + hill + mount + div);

#ifdef FLOODTESTMAP
			// custom map
			total = (abs(x-500) + abs(y-500))/20.0f;
#endif

			const float sandThreshold = noises[NoiseType_Sand].noise(x, y, m_params.noiseSampleHeight) * m_params.sandHeightVariance + m_params.minimumSandHeight;

			if (total < sandThreshold)
			{
				// Sand (1.5g/cm3)
				m_nodes[y * width + x].addMarker(glm::max(BEDROCK_SAFETY_LAYER, total), m_params.sandResistivity, false, glm::vec3(1.0f, 1.0f, 0.7f), m_params.sandFertility, 1.0f, 0.0f, m_maxHeight);
			}
			else
			{
				// Topsoil (2.3g/cm3). Clay will make more resistive, sand will make less resistive.
				float topNoise = noises[NoiseType_Resistivity].noise(x / (m_params.soilResistivityChangeRate / m_params.scale), y / (m_params.soilResistivityChangeRate / m_params.scale), glm::max(BEDROCK_SAFETY_LAYER, total));
				float sandAmount = m_params.soilSandContent + (1.0f - topNoise) * m_params.soilSandVariance;
				float clayAmount = m_params.soilClayContent + topNoise * m_params.soilSandContent;
				float resistivity = m_params.soilResistivityBase + topNoise * m_params.soilResistivityVariance;
				m_nodes[y * width + x].addMarker(glm::max(BEDROCK_SAFETY_LAYER, total), resistivity, false, glm::vec3(0.2f + topNoise * 0.4f, 0.3f, 0.0f), m_params.soilFertility, sandAmount, clayAmount, m_maxHeight);
			}
			// Bedrock (7.5g/cm3)
			m_nodes[y * width + x].addMarker(BEDROCK_LAYER, m_params.bedrockResisitivity, true, glm::vec3(0.1f), 0.0f, 0.0f, 0.0f, m_maxHeight);
			// Fill all nodes to a basic "sea level"
			m_nodes[y * width + x].setWaterHeight(m_params.seaLevel);
		}

		// Display progress
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
	float incrementValue = 1.0f / (float)m_params.generatedMapDensity;

	for (int x = 0; x < m_width; ++x)
	{
		for (int y = 0; y < m_height; ++y)
		{
			bool isRock = false;
			float height = getHeightAt(x, y);

			float maxHeightScaled = height / m_maxHeight;

			// Place a tree
			if (rand() % m_params.treeGenerationRarity == 0)
				trySpawnTree(glm::vec2(x, y));

			// Peak heights can be springs, spawning water constantly
			if (maxHeightScaled >= m_params.springThreshold && height > m_params.minimumSpringHeight &&  rand() % m_params.springRarity == 0)
				addSpring(x, y);

			for (float currHeight = 0.0f; currHeight < maxHeightScaled; currHeight += incrementValue)
			{
				if (currHeight * m_maxHeight < BEDROCK_SAFETY_LAYER)
					continue;

				const float scaledDensHeight = currHeight * m_params.rockVerticalScaling;
				if (!isRock)
				{
					// rock resistiveForce (3.8-4.2g/cm3)
					float currVal = rockNoise->noise(x / (m_params.rockRarity / m_params.scale), y / (m_params.rockRarity / m_params.scale), scaledDensHeight);
					if (currVal > m_params.rockThreshold)
					{
						float resistivity = m_params.rockResistivityBase + (currVal - m_params.rockThreshold) * m_params.rockResistivityVariance;
						m_nodes[y * m_width + x].addMarker(currHeight * m_maxHeight, resistivity, true, glm::vec3(0.1f, 0.1f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.5f) * currVal, 0.0f, 0.0f, 0.0f, m_maxHeight);
						isRock = true;
					}
					else
					{
						// soil resistiveForce (2.3-2.6g/cm3, increasing with depth)
						float noise = resistivityNoise->noise(x / (m_params.soilResistivityChangeRate / m_params.scale), y / (m_params.soilResistivityChangeRate / m_params.scale), scaledDensHeight);
						float resistivity = m_params.soilResistivityBase + glm::max(0.0f, 0.5f - currHeight) + noise * m_params.soilResistivityVariance;
						float sandAmount = m_params.soilSandContent + noise * m_params.soilSandVariance;
						float clayAmount = m_params.soilClayContent + noise * m_params.soilClayVariance;
						glm::vec3 col = glm::vec3(0.2f + noise * 0.2f, 0.3f, 0.0f);
						m_nodes[y * m_width + x].addMarker(currHeight * m_maxHeight, resistivity, false, col, m_params.soilFertility, sandAmount, clayAmount, m_maxHeight);
					}
				}
				else
				{
					// rock resistiveForce (3.8-4.2g/cm3)
					float currVal = rockNoise->noise(x / (m_params.rockRarity / m_params.scale), y / (m_params.rockRarity / m_params.scale), scaledDensHeight);
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
	m_soilDefinitions.push_back(SoilDefinition("eapa", glm::vec2(0.22f, 0.29f), glm::vec2(0.24f, 0.34f), glm::vec2(0.2f, 0.8f), glm::vec2(0.0f, 2.5f)));
	m_soilDefinitions.push_back(SoilDefinition("attewan", glm::vec2(0.05f, 0.28f), glm::vec2(0.35f, 0.79f), glm::vec2(0.2f, 0.75f), glm::vec2(2.2f, 3.0f)));
	m_soilDefinitions.push_back(SoilDefinition("ethridge", glm::vec2(0.31f, 0.39f), glm::vec2(0.18f, 0.30f), glm::vec2(0.2f, 0.8f), glm::vec2(2.8f, 4.0f)));
	m_soilDefinitions.push_back(SoilDefinition("yamacall", glm::vec2(0.23f, 0.26f), glm::vec2(0.30f, 0.40f), glm::vec2(0.85f, 1.0f), glm::vec2(1.0f, 2.45f)));
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
	return hillVal * hillHeight / m_params.scale;
}

float Map::getDivetValue(PerlinNoise* noise, int x, int y, float divetHeight, float rarity)
{
	glm::vec2 XY = calculateXYFromRarity(x, y, rarity);
	float divetVal = -(float)noise->noise(XY.x, XY.y, m_params.noiseSampleHeight);
	return divetVal * divetHeight / m_params.scale;
}

float Map::getMountainValue(PerlinNoise* noise, int x, int y, float mountainHeight, float rarity)
{
	glm::vec2 XY = calculateXYFromRarity(x, y, rarity);
	float mountainVal = 0.0f;
	bool mountain = noise->noise(XY.x, XY.y, m_params.noiseSampleHeight) > m_params.mountainThreshold;
	if (mountain)
	{
		mountainVal += pow((noise->noise(XY.x, XY.y, m_params.noiseSampleHeight) - m_params.mountainThreshold) * sqrt(mountainHeight) * m_params.mountainConstantMultiplier / m_params.scale, 2);
	}
	return mountainVal;
}

glm::vec2 Map::calculateXYFromRarity(int x, int y, float rarity)
{
	float scalar = (rarity / m_params.scale);
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

std::string Map::getMapGeneralSoilType()
{
	std::ostringstream oss;
	oss << "Soil types:" << std::endl;
	int* count = new int[m_soilDefinitions.size()];
	std::fill(count, count + m_soilDefinitions.size(), 0);
	float certainty;

	for (int i = 0; i < m_width * m_height; ++i)
	{
		NodeMarker general = m_nodes[i].getDataAboveHeight(m_nodes[i].topHeight() - 1.0f, true);
		int index = getSoilTypeBestMatching(&general, certainty);
		if (index != -1)
			count[index]++;
	}

	for (int i = 0; i < m_soilDefinitions.size(); ++i)
	{
		oss << m_soilDefinitions[i].name << ": " << count[i] * 100 / (m_width * m_height) << "%" << std::endl;
	}

	delete[m_soilDefinitions.size()] count;
	return oss.str();
}

int Map::getSoilTypeBestMatching(NodeMarker* nodeData, float& bestCertainty)
{
	int bestIndex = -1;
	bestCertainty = 0.0f;

	for (int i = 0; i < m_soilDefinitions.size(); i++)
	{
		float certainty = m_soilDefinitions[i].getCertainty(nodeData);
		if (certainty > bestCertainty)
		{
			bestCertainty = certainty;
			bestIndex = i;
		}
	}

	return bestIndex;
}

std::string Map::getSoilType(glm::vec2 pos)
{
	std::ostringstream oss;
	Node* node = getNodeAt(pos.x, pos.y);
	int bestIndex = -1;
	float bestCertainty = 0.0f;
	NodeMarker general = node->getDataAboveHeight(-1.9f, true);

	bestIndex = getSoilTypeBestMatching(&general, bestCertainty);

	if (bestIndex != -1)
	{
		oss << m_soilDefinitions[bestIndex].name << " (" << bestCertainty << " % certainty)";
	}
	else
	{
		oss << "unknown";
	}

	NodeMarker loam = node->getDataAboveHeight(node->topHeight() - 1.0f, true);
	int mainIndex = bestIndex;
	bestIndex = getSoilTypeBestMatching(&loam, bestCertainty);

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

void Map::erode(int cycles) 
{
	m_age++;
	// Track all particle movement
	bool* track = new bool[m_width * m_height];
	std::fill(track, track + m_width * m_height, false);
	glm::vec2 dim = glm::vec2(m_width, m_height);
	int springIndex = 0;
	float completion = 0.0f;

	for (int currentCycle = 0; currentCycle < cycles; currentCycle++)
	{
		// Spawn particle
		glm::vec2 newParticlePos = glm::vec2(rand() % m_width, rand() % m_height);

		// Spawn at spring if possible
		if (springIndex < m_springs.size())
		{
			newParticlePos = m_springs.at(springIndex);
			springIndex++;
		}

		Drop drop(newParticlePos, &m_params);

		// If we've moved 1km, give up.
		while (drop.getVolume() > drop.getMinVolume() && drop.getAge() < 1000) {

			if (!drop.descend(normal((int)drop.getPosition().y * m_width + (int)drop.getPosition().x), m_nodes, track, dim, m_maxHeight) && drop.getVolume() > drop.getMinVolume())
			{
				if (!drop.flood(m_nodes, dim, m_maxHeight))
					break;
			}
		}

		// If we've terminated for whatever reason, immediately try and flood
		if (drop.getAge() >= 1000)
			drop.flood(m_nodes, dim, m_maxHeight);

		float prevCompletion = completion;
		completion = (currentCycle / (float)cycles) * 100.0f;
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

	// Travelled nodes can be filled outwards for wider, more effective-looking rivers
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

			m_nodes[i].setParticles(glm::max(0.0f, m_nodes[i].getParticles() * (1.0f - (0.5f * m_params.streamEvaporationRate))));
		}
		else
		{
			m_nodes[i].setParticles(glm::max(0.0f, m_nodes[i].getParticles() * (1.0f - m_params.streamEvaporationRate)));
		}
	}

	delete[m_width * m_height] track;
}

void Map::grow()
{
	// Spawn a tree randomly on the map (long-distance fertilization)
	for (int i = 0; i < m_params.treeLongDistanceFertilizationCount; i++)
	{
		int newTreePos = rand() % (m_width * m_height);
		trySpawnTree(glm::vec2(newTreePos % m_width, newTreePos / m_width));
	}

	int completion = 0;

	for (int i = 0; i < m_width * m_height; i++)
	{
		// Tree spawns a new tree
		if (m_nodes[i].getFoliageDensity() > 0.5f)
		{
			if (rand() % m_params.treeSpreadChance == 0)
			{
				glm::vec2 newPlantPos = glm::vec2(i % m_width, i / m_width) + glm::vec2(rand() % m_params.treeSpreadRadius - (m_params.treeSpreadRadius / 2), rand() % m_params.treeSpreadRadius - (m_params.treeSpreadRadius / 2));
				trySpawnTree(newPlantPos);
			}

			// Trees die in water & sometimes die randomly
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