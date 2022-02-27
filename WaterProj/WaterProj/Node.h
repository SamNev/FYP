#pragma once

#include <GL/glew.h>
#include <glm.hpp>
#include <vector>

struct NodeMarker
{
	float height;
	float density;
	bool hardStop;
	glm::vec3 color;
};

class Node {
public:
	void addMarker(float height, float density, bool hardStop, glm::vec3 color, float& maxHeight);
	float getDensityAtHeight(float height);
	NodeMarker* top();
protected:
	std::vector<NodeMarker> m_nodeData;
};