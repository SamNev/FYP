#include "MapRenderer.h"

#include <ext.hpp>
#include <fstream>
#include <GL/glew.h>
#include <glm.hpp>
#include <Windows.h>

#include "Map.h"
#include "ShaderProgram.h"

MapRenderer::MapRenderer(Map* map)
{
	m_map = map;
	m_groundRenderer = createShaderProgram("vertex.txt", "fragment.txt");
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

void MapRenderer::render(SDL_Window* window)
{
	m_groundRenderer->use(); 
	glClearColor(0.0f, 0.2f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(m_vaoId);
	glViewport(0, 0, 900, 900);
	glEnable(GL_DEPTH_TEST);
	GLuint color = m_groundRenderer->getUniform("u_Colour");
	GLuint surrounding = m_groundRenderer->getUniform("u_Surrounding");
	GLuint pos = m_groundRenderer->getUniform("u_Pos");
	GLuint proj = m_groundRenderer->getUniform("u_Proj");
	GLuint view = m_groundRenderer->getUniform("u_View");
	glm::mat4 projMat = glm::perspective(glm::radians(45.0f), 900.0f / 900.0f, 0.1f, 1000.f);
	glm::mat4 viewMat = glm::lookAt(glm::vec3(0, 12.0f + m_map->getScale(), 0), glm::vec3(1, 12.0f + m_map->getScale(), 1), glm::vec3(0, 1, 0));
	glUniformMatrix4fv(proj, 1, GL_FALSE, glm::value_ptr(projMat));
	glUniformMatrix4fv(view, 1, GL_FALSE, glm::value_ptr(viewMat));

	for (int x = 0; x < m_map->getWidth(); ++x)
	{
		for (int y = 0; y < m_map->getHeight(); ++y)
		{
			// calc model matrix here
			const float height = m_map->getHeightAt(x, y);
			const glm::vec3 current = { x, height, y };
			glm::mat4 model = glm::translate(glm::mat4(1.0f), current);
			const float right = m_map->getHeightAt(x + 1, y);
			const float left = m_map->getHeightAt(x - 1, y);
			const float down = m_map->getHeightAt(x, y + 1);
			const float up = m_map->getHeightAt(x, y - 1);
			const float rightUp = m_map->getHeightAt(x + 1, y - 1);
			const float rightDown = m_map->getHeightAt(x + 1, y + 1);
			const float leftUp = m_map->getHeightAt(x - 1, y - 1);
			const float leftDown = m_map->getHeightAt(x - 1, y + 1);

			const float topRightHeight = ((up + right + rightUp + height) / 4.0f) - height;
			const float bottomRightHeight = ((down + right + rightDown + height) / 4.0f) - height;
			const float bottomLeftHeight = ((down + left + leftDown + height) / 4.0f) - height;
			const float topLeftHeight = ((up + left + leftUp + height) / 4.0f) - height;

			//glUniform4f(surrounding, topRightHeight, bottomLeftHeight, bottomRightHeight, topLeftHeight);
			glUniform4f(surrounding, topRightHeight, bottomLeftHeight, topLeftHeight, bottomRightHeight);
			glUniform3f(color, 0.0f, 1.0f, 0.0f);
			glUniformMatrix4fv(pos, 1, GL_FALSE, glm::value_ptr(model));

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