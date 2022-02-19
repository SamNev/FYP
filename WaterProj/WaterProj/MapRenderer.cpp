#include "MapRenderer.h"

#include <fstream>
#include <GL/glew.h>
#include <Windows.h>

#include "Map.h"
#include "ShaderProgram.h"

MapRenderer::MapRenderer(Map* map)
{
	m_map = map;
	m_groundRenderer = createShaderProgram("vertex.txt", "fragment.txt");
}

MapRenderer::~MapRenderer()
{
	for (int i = 0; i < m_shaderPrograms.size(); i++)
	{
		delete(m_shaderPrograms[i]);
	}

	m_shaderPrograms.clear();
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

void MapRenderer::render()
{
	m_groundRenderer->use(); 
	glClearColor(0.0f, 0.2f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (int x = 0; x < m_map->getWidth(); ++x)
	{
		for (int y = 0; y < m_map->getHeight(); ++y)
		{
			// calc model matrix here
			const glm::vec3 current = { x, y, m_map->getHeightAt(x,y) };
			const float down = m_map->getHeightAt(x, y + 1);
			const float up = m_map->getHeightAt(x, y - 1);
			const float right = m_map->getHeightAt(x + 1, y);
			const float left = m_map->getHeightAt(x - 1, y);

			glBegin(GL_POINTS);
			glVertex3f(x, m_map->getHeightAt(x,y), y);
			glEnd();

		}
	}
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