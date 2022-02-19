#pragma once

#include <GL/glew.h>
#include <string>
#include <vector>

class Map;
class ShaderProgram;

class MapRenderer
{
public:
	MapRenderer(Map* map);
	~MapRenderer();
	ShaderProgram* createShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath);
	void render();
	void calcPath(std::string& path);
protected:
	std::vector<ShaderProgram*> m_shaderPrograms;
	ShaderProgram* m_groundRenderer;
	Map* m_map;
	std::string m_knownResourceFolder;
};