#pragma once

#include <ext.hpp>
#include <glm.hpp>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <string>
#include <vector>

class Map;
class ShaderProgram;

/***************************************************************************//**
 * The map renderer refers to the map and provides an OpenGL wrapper for all
 * rendering purposes. In theory, this could be swapped out for any renderer.
 ******************************************************************************/
class MapRenderer
{
public:
	/***************************************************************************//**
	 * Creates a mapRenderer referring to the current map.
	 @params map The map to render
	 ******************************************************************************/
	MapRenderer(Map* map);
	~MapRenderer();
	/***************************************************************************//**
	 * Makes a shader program based on the fragment and vertex shaders loaded from files
	 @params vertexShaderPath The file path of the vertex shader txt
	 @params fragmentShaderPath The file path of the fragment shader txt
	 ******************************************************************************/
	ShaderProgram* createShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath);
	/***************************************************************************//**
	 * Perform a full render pass at the current camera position, rendering to the given window.
	 @params window The window to render to
	 ******************************************************************************/
	void render(SDL_Window* window);
	/***************************************************************************//**
	 * Perform a render pass at the current camera position, rendering only at a
	 * given height. For segmenting terrain and debug views.
	 @params window The window to render to
	 @paramas height The height to render at
	 ******************************************************************************/
	void renderAtHeight(SDL_Window* window, float height);
	/***************************************************************************//**
	 * Calculates the path for the file name specified by path. Will be adjusted as needed.
	 @params path The name of the file (with extension) to search for.
	 ******************************************************************************/
	void calcPath(std::string& path);
	void makeMapTile();
	void transformCam(glm::vec2 transformation);
	void setMap(Map* map);
	void setCamPos(glm::vec3 camPos) { m_camPos = camPos; }
	glm::vec3 getCamPos() { return m_camPos; }
	void zoomIn();
	void zoomOut();
	float getCullDist();
	/***************************************************************************//**
	 * Works out the position defined by pos's distance from the camera. For culling.
	 @params pos The position to compare to the camera's position
	 ******************************************************************************/
	float distFromCamera(glm::vec3 pos);
	int lodScaling();
	int uncappedLodScaling();
	/***************************************************************************//**
	 * Cache all shader program properties
	 ******************************************************************************/
	void cacheProperties();
	/***************************************************************************//**
	 * Calculate view matrix etc for rendering. Sets the ignore height flag on the
	 * fragment shader.
	 @params ignoreHeight Whether this render pass should render heigher terrain brighter.
	 ******************************************************************************/
	void prepareRender(bool ignoreHeight);
protected:
	std::vector<ShaderProgram*> m_shaderPrograms;
	ShaderProgram* m_groundRenderer;
	Map* m_map;
	GLuint m_vaoId;
	std::string m_knownResourceFolder;
	glm::vec3 m_camPos;
	int m_zoomLevel = 1;
	bool m_propertiesCached = false;

	GLuint m_colorLoc;
	GLuint m_surroundingLoc;
	GLuint m_surroundingColorLoc;
	GLuint m_posLoc;
	GLuint m_maxHeightLoc;
	GLuint m_ignoreHeightLoc;
	GLuint m_surrWaterHeightLoc;
	GLuint m_projLoc;
	GLuint m_viewLoc;
};