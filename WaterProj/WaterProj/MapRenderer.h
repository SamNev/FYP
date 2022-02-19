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
	void makeMapTile();
protected:
	std::vector<ShaderProgram*> m_shaderPrograms;
	ShaderProgram* m_groundRenderer;
	Map* m_map;
	GLuint m_vaoId;
	std::string m_knownResourceFolder;
};