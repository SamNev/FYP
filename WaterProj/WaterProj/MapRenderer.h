#pragma once

#include <ext.hpp>
#include <glm.hpp>
#include <GL/glew.h>
#include "SDL2/SDL.h"
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
	void render(SDL_Window* window);
	void renderWater(SDL_Window* window);
	void renderAtHeight(SDL_Window* window, float height);
	void calcPath(std::string& path);
	void makeMapTile();
	void transformCam(glm::vec2 transformation);
	void setMap(Map* map);
	void zoomIn();
	void zoomOut();
	float getCullDist();
	float distFromCamera(glm::vec3 pos);
	int lodScaling();
	int uncappedLodScaling();
protected:
	std::vector<ShaderProgram*> m_shaderPrograms;
	ShaderProgram* m_groundRenderer;
	ShaderProgram* m_waterRenderer;
	Map* m_map;
	GLuint m_vaoId;
	std::string m_knownResourceFolder;
	glm::vec3 m_camPos;
	int m_zoomLevel = 1;
	GLuint m_fbo;
	GLuint m_tex;
};