#include "MapRenderer.h"

#include <fstream>
#include <GL/glew.h>
#include <Windows.h>

#include "Map.h"
#include "Node.h"
#include "ShaderProgram.h"

MapRenderer::MapRenderer(Map* map)
{
	m_map = map;
	m_camPos = glm::vec3(0.0f, m_map->getMaxHeight(), 0.0f);
	m_groundRenderer = createShaderProgram("vertex.txt", "fragment.txt");
	m_zoomLevel = 50;
	makeMapTile();
}

MapRenderer::~MapRenderer()
{
	for (int i = 0; i < m_shaderPrograms.size(); i++)
	{
		delete(m_shaderPrograms[i]);
	}

	m_shaderPrograms.clear();
}

void MapRenderer::setMap(Map* map)
{
	m_map = map;
	m_zoomLevel = 50;
	m_camPos = glm::vec3(0.0f, m_map->getMaxHeight(), 0.0f);
}

void MapRenderer::makeMapTile()
{
	const GLfloat positionsA[] = {
		0.0f, 0.0f, 0.0f,
		0.5f, 0.0f, 0.5f,
		0.5f, 0.0f, -0.5f,

		0.0f, 0.0f, 0.0f,
		0.5f, 0.0f, -0.5f,
		-0.5f, 0.0f, -0.5f,

		0.0f, 0.0f, 0.0f,
		-0.5f, 0.0f, -0.5f,
		-0.5f, 0.0f, 0.5f,

		0.0f, 0.0f, 0.0f,
		-0.5f, 0.0f, 0.5f,
		0.5f, 0.0f, 0.5f
	};

	// VBO config
	GLuint positionsVboId = 0;

	// Create a new VBO on the GPU and bind it
	glGenBuffers(1, &positionsVboId);

	if (!positionsVboId)
	{
		throw std::exception("VBO Generation failed!");
	}

	glBindBuffer(GL_ARRAY_BUFFER, positionsVboId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positionsA), positionsA, GL_STATIC_DRAW);
	// Reset the state
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//VAO config
	m_vaoId = 0;

	// Create a new VAO on the GPU and bind it
	glGenVertexArrays(1, &m_vaoId);

	if (!m_vaoId)
	{
		throw std::exception("VAO Generation failed");
	}

	int size = sizeof(positionsA) / sizeof(positionsA[0]);
	glBindVertexArray(m_vaoId);

	// Bind the position VBO, assign it to position 0 on the bound VAO
	// and flag it to be used
	glBindBuffer(GL_ARRAY_BUFFER, positionsVboId);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		3 * sizeof(GLfloat), (void*)0);

	// Reset the state
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

ShaderProgram* MapRenderer::createShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath)
{
	calcPath(vertexShaderPath);
	std::ifstream vertexFile(vertexShaderPath);
	GLchar* vShaderText = NULL;

	if (vertexFile.is_open())
	{
		vertexFile.seekg(0, vertexFile.end);
		int length = (int)vertexFile.tellg() + 1;
		vertexFile.seekg(0, vertexFile.beg);

		// Create our buffer
		vShaderText = new char[length] { NULL };

		// Transfer data from file to buffer
		vertexFile.read(&vShaderText[0], length);

		// End of the file
		if (!vertexFile.eof())
		{
			vertexFile.close();
			throw std::exception((std::string("Could not read vertex shader from file: ") + vertexShaderPath).c_str());
		}

		// Needs to be NULL-terminated
		vShaderText[length - 1] = 0;

		vertexFile.close();
	}
	else
	{
		throw std::exception((std::string("Could not open vertex shader from file: ") + vertexShaderPath).c_str());
	}

	calcPath(fragmentShaderPath);
	std::ifstream fragmentFile(fragmentShaderPath);
	GLchar* fShaderText = NULL;

	if (fragmentFile.is_open())
	{
		fragmentFile.seekg(0, fragmentFile.end);
		int length = (int)fragmentFile.tellg() + 1;
		fragmentFile.seekg(0, fragmentFile.beg);

		// Create our buffer
		fShaderText = new char[length] { NULL };
		// Transfer data from file to buffer
		fragmentFile.read(&fShaderText[0], length);

		// End of the file
		if (!fragmentFile.eof())
		{
			fragmentFile.close();
			throw std::exception((std::string("Could not read fragment shader from file: ") + fragmentShaderPath).c_str());
		}

		// Needs to be NULL-terminated
		fShaderText[length - 1] = 0;

		fragmentFile.close();
	}
	else
	{
		throw std::exception((std::string("Could not open fragment shader from file: ") + fragmentShaderPath).c_str());
	}

	ShaderProgram* prog = new ShaderProgram(vShaderText, fShaderText);
	m_shaderPrograms.push_back(prog);
	return prog;
}

int MapRenderer::lodScaling()
{
	return min(8.0f, uncappedLodScaling());
}

int MapRenderer::uncappedLodScaling()
{
	return max((int)(sqrt(m_zoomLevel) * 1.1f) + ((m_zoomLevel == 1) ? 0.0f : 1.0f), 1);
}

void MapRenderer::transformCam(glm::vec2 transformation)
{
	m_camPos = glm::vec3(m_camPos.x + transformation.x, m_camPos.y, m_camPos.z + transformation.y);
}

float MapRenderer::getCullDist()
{
	return min(pow(uncappedLodScaling(), 1.9f) * 50.0f, 4000.0f);
}

float MapRenderer::distFromCamera(glm::vec3 pos)
{
	return glm::length(m_camPos - pos);
}

void MapRenderer::render(SDL_Window* window)
{
	const int lodScale = lodScaling();
	m_groundRenderer->use(); 
	glClearColor(0.0f, 0.2f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(m_vaoId);
	glViewport(0, 0, 900, 900);
	glEnable(GL_DEPTH_TEST);
	// TODO: cache! Also analyse this file
	GLuint colorLoc = m_groundRenderer->getUniform("u_Color");
	GLuint surroundingLoc = m_groundRenderer->getUniform("u_Surrounding");
	GLuint surroundingColorLoc = m_groundRenderer->getUniform("u_SurroundingColor");
	GLuint posLoc = m_groundRenderer->getUniform("u_Pos");
	GLuint maxHeightLoc = m_groundRenderer->getUniform("u_MaxHeight");
	GLuint igHeightLoc = m_groundRenderer->getUniform("u_IgnoreHeight");
	GLuint surrWaterHeightLoc = m_groundRenderer->getUniform("u_SurroundingWaterHeight");
	GLuint proj = m_groundRenderer->getUniform("u_Proj");
	GLuint view = m_groundRenderer->getUniform("u_View");
	glm::mat4 projMat = glm::perspective(glm::radians(45.0f), 900.0f / 900.0f, 0.1f, getCullDist());
	m_camPos.y = m_map->getHeightAt(m_camPos.x, m_camPos.z) + (m_zoomLevel * 2);
	glm::mat4 viewMat = glm::lookAt(m_camPos, glm::vec3(m_camPos.x + 1.0f, m_camPos.y, m_camPos.z + 1.0f), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(proj, 1, GL_FALSE, glm::value_ptr(projMat));
	glUniformMatrix4fv(view, 1, GL_FALSE, glm::value_ptr(viewMat));
	glUniform1f(maxHeightLoc, m_map->getMaxHeight());
	glUniform1i(igHeightLoc, false);
	const float cullDist = getCullDist();

	for (int x = 0; x < m_map->getWidth(); x += lodScale)
	{
		for (int y = 0; y < m_map->getHeight(); y += lodScale)
		{
			// calc model matrix here
			const float height = m_map->getHeightAt(x, y);
			glm::vec3 color = m_map->getNodeAt(x, y)->topColor();

			const glm::vec3 current = { x, height, y };
			if (cullDist < distFromCamera(current))
				continue;

			if (current.x < m_camPos.x && current.y < m_camPos.y)
				continue;

			glm::mat4 model = glm::translate(glm::mat4(1.0f), current);
			model = glm::scale(model, glm::vec3(lodScale, 1.0f, lodScale));
			const Node* right = m_map->getNodeAt(x + lodScale, y);
			const Node* left = m_map->getNodeAt(x - lodScale, y);
			const Node* down = m_map->getNodeAt(x, y + lodScale);
			const Node* up = m_map->getNodeAt(x, y - lodScale);
			const Node* rightUp = m_map->getNodeAt(x + lodScale, y - lodScale);
			const Node* rightDown = m_map->getNodeAt(x + lodScale, y + lodScale);
			const Node* leftUp = m_map->getNodeAt(x - lodScale, y - lodScale);
			const Node* leftDown = m_map->getNodeAt(x - lodScale, y + lodScale);
			
			const float topRightHeight = ((up->topHeight() + right->topHeight() + rightUp->topHeight() + height) / 4.0f) - height;
			const float bottomRightHeight = ((down->topHeight() + right->topHeight() + rightDown->topHeight() + height) / 4.0f) - height;
			const float bottomLeftHeight = ((down->topHeight() + left->topHeight() + leftDown->topHeight() + height) / 4.0f) - height;
			const float topLeftHeight = ((up->topHeight() + left->topHeight() + leftUp->topHeight() + height) / 4.0f) - height;
			
			const glm::vec3 topRightColor = ((up->topColor() + right->topColor() + rightUp->topColor() + color) / 4.0f);
			const glm::vec3 bottomRightColor = ((down->topColor() + right->topColor() + rightDown->topColor() + color) / 4.0f);
			const glm::vec3 bottomLeftColor = ((down->topColor() + left->topColor() + leftDown->topColor() + color) / 4.0f);
			const glm::vec3 topLeftColor = ((up->topColor() + left->topColor() + leftUp->topColor() + color) / 4.0f);
			float colors[12] = { topRightColor.x, topRightColor.y, topRightColor.z, bottomLeftColor.x, bottomLeftColor.y, bottomLeftColor.z, topLeftColor.x, topLeftColor.y, topLeftColor.z, bottomRightColor.x, bottomRightColor.y, bottomRightColor.z};
			
			for (int i = 0; i < 12; ++i)
			{
				if (colors[i] < 0)
					colors[i] = 0;

				if (colors[i] > 1)
					colors[i] = 1;
			}

			float waterHeight = m_map->getNodeAt(x, y)->waterHeight(-100.0f);
			if (waterHeight == -100.0f)
			{
				waterHeight = 0.0f;

				const float max = max(max(up->waterHeight(waterHeight), down->waterHeight(waterHeight)), max(left->waterHeight(waterHeight), right->waterHeight(waterHeight)));
				glUniform4f(surrWaterHeightLoc, max, max, max, max);
			}

			{
				const float topRightWater = ((up->waterHeight(waterHeight) + right->waterHeight(waterHeight) + rightUp->waterHeight(waterHeight) + waterHeight) / 4.0f);
				const float bottomRightWater = ((down->waterHeight(waterHeight) + right->waterHeight(waterHeight) + rightDown->waterHeight(waterHeight) + waterHeight) / 4.0f);
				const float bottomLeftWater = ((down->waterHeight(waterHeight) + left->waterHeight(waterHeight) + leftDown->waterHeight(waterHeight) + waterHeight) / 4.0f);
				const float topLeftWater = ((up->waterHeight(waterHeight) + left->waterHeight(waterHeight) + leftUp->waterHeight(waterHeight) + waterHeight) / 4.0f);
				glUniform4f(surrWaterHeightLoc, topRightWater, bottomLeftWater, topLeftWater, bottomRightWater);
			}
			
			glUniform3fv(surroundingColorLoc, 4, colors);
			glUniform4f(surroundingLoc, topRightHeight, bottomLeftHeight, topLeftHeight, bottomRightHeight);
			glUniform3f(colorLoc, color.x, color.y, color.z);
			glUniformMatrix4fv(posLoc, 1, GL_FALSE, glm::value_ptr(model));

			glDrawArrays(GL_TRIANGLES, 0, 12);
		}
	}

	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(0);

	SDL_GL_SwapWindow(window);
}

void MapRenderer::renderAtHeight(SDL_Window* window, float height)
{
	const int lodScale = lodScaling();
	m_groundRenderer->use();
	glClearColor(0.0f, 0.2f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(m_vaoId);
	glViewport(0, 0, 900, 900);
	glEnable(GL_DEPTH_TEST);
	GLuint colorLoc = m_groundRenderer->getUniform("u_Color");
	GLuint surroundingLoc = m_groundRenderer->getUniform("u_Surrounding");
	GLuint surroundingColorLoc = m_groundRenderer->getUniform("u_SurroundingColor");
	GLuint posLoc = m_groundRenderer->getUniform("u_Pos");
	GLuint maxHeightLoc = m_groundRenderer->getUniform("u_MaxHeight");
	GLuint igHeightLoc = m_groundRenderer->getUniform("u_IgnoreHeight");
	GLuint proj = m_groundRenderer->getUniform("u_Proj");
	GLuint view = m_groundRenderer->getUniform("u_View");
	glm::mat4 projMat = glm::perspective(glm::radians(45.0f), 900.0f / 900.0f, 0.1f, getCullDist());
	m_camPos.y = m_map->getHeightAt(m_camPos.x, m_camPos.z) + (m_zoomLevel * 2);
	glm::mat4 viewMat = glm::lookAt(m_camPos, glm::vec3(m_camPos.x + 1.0f, m_camPos.y, m_camPos.z + 1.0f), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(proj, 1, GL_FALSE, glm::value_ptr(projMat));
	glUniformMatrix4fv(view, 1, GL_FALSE, glm::value_ptr(viewMat));
	glUniform1f(maxHeightLoc, m_map->getMaxHeight());
	glUniform1i(igHeightLoc, true);
	const float cullDist = getCullDist();

	for (int x = 0; x < m_map->getWidth(); x += lodScale)
	{
		for (int y = 0; y < m_map->getHeight(); y += lodScale)
		{
			if (height > m_map->getHeightAt(x, y))
				continue;

			// calc model matrix here
			const glm::vec3 current = { x, height, y };
			if (cullDist < distFromCamera(current))
				continue;

			glm::mat4 model = glm::translate(glm::mat4(1.0f), current);
			model = glm::scale(model, glm::vec3(lodScale, 1.0f, lodScale));

			const Node* right = m_map->getNodeAt(x + lodScale, y);
			const Node* left = m_map->getNodeAt(x - lodScale, y);
			const Node* down = m_map->getNodeAt(x, y + lodScale);
			const Node* up = m_map->getNodeAt(x, y - lodScale);
			const Node* rightUp = m_map->getNodeAt(x + lodScale, y - lodScale);
			const Node* rightDown = m_map->getNodeAt(x + lodScale, y + lodScale);
			const Node* leftUp = m_map->getNodeAt(x - lodScale, y - lodScale);
			const Node* leftDown = m_map->getNodeAt(x - lodScale, y + lodScale); 

			const glm::vec3 color = m_map->getNodeAt(x, y)->getColorAtHeight(height);
			const glm::vec3 topRightColor = ((up->getColorAtHeight(height) + right->getColorAtHeight(height) + rightUp->getColorAtHeight(height) + color) / 4.0f);
			const glm::vec3 bottomRightColor = ((down->getColorAtHeight(height) + right->getColorAtHeight(height) + rightDown->getColorAtHeight(height) + color) / 4.0f);
			const glm::vec3 bottomLeftColor = ((down->getColorAtHeight(height) + left->getColorAtHeight(height) + leftDown->getColorAtHeight(height) + color) / 4.0f);
			const glm::vec3 topLeftColor = ((up->getColorAtHeight(height) + left->getColorAtHeight(height) + leftUp->getColorAtHeight(height) + color) / 4.0f);

 			const float values[12] = { topRightColor.x, topRightColor.y, topRightColor.z, bottomLeftColor.x, bottomLeftColor.y, bottomLeftColor.z, topLeftColor.x, topLeftColor.y, topLeftColor.z, bottomRightColor.x, bottomRightColor.y, bottomRightColor.z };

			glUniform3fv(surroundingColorLoc, 4, values);

			glUniform4f(surroundingLoc, 0.0f, 0.0f, 0.0f, 0.0f);
			glUniform3f(colorLoc, color.x, color.y, color.z);
			glUniformMatrix4fv(posLoc, 1, GL_FALSE, glm::value_ptr(model));

			glDrawArrays(GL_TRIANGLES, 0, 12);
		}
	}

	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(0);

	SDL_GL_SwapWindow(window);
}

void MapRenderer::calcPath(std::string& path)
{
	bool found = false;
	// If we know where it's going to be, just fetch from the resource folder
	if (m_knownResourceFolder != "")
	{
		std::ifstream file(m_knownResourceFolder + path);
		if (file.is_open())
		{
			file.close();
			path = m_knownResourceFolder + path;
			found = true;
		}
	}

	// If we don't know, or didn't find it in the resource folder, try and find it.
	if (!found)
	{
		wchar_t strExePath[MAX_PATH];
		GetModuleFileName(NULL, strExePath, MAX_PATH);
		size_t origsize = wcslen(strExePath) + 1;
		size_t convertedChars = 0;
		const size_t newsize = origsize * 2;
		char* nstring = new char[newsize];
		wcstombs_s(&convertedChars, nstring, newsize, strExePath, _TRUNCATE);

		std::string folderPath = std::string(nstring);
		std::string executableName = folderPath.substr(folderPath.find_last_of("\\"));
		executableName = executableName.substr(1, executableName.length() - 5);

		while (!found && folderPath.find_last_of("\\") != std::string::npos)
		{
			folderPath.erase(folderPath.find_last_of("\\"));
			std::string fullpath = folderPath + "\\" + executableName + "\\" + path;
			std::ifstream file(fullpath);
			if (file.is_open())
			{
				file.close();
				// We found something here, so look here in future.
				m_knownResourceFolder = folderPath + "\\" + executableName + "\\";
				path = fullpath;
				found = true;
			}
		}
	}
}

void MapRenderer::zoomIn()
{
	if (m_zoomLevel > 1)
	{
		m_zoomLevel--;
		m_camPos.y -= m_zoomLevel;
	}
}

void MapRenderer::zoomOut()
{
	m_zoomLevel++;
	m_camPos.y += m_zoomLevel;
}