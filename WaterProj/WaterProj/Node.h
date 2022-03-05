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
	void erodeByValue(float amount);
	float getDensityAtHeight(float height) const;
	glm::vec3 getColorAtHeight(float height) const;
	float topHeight() const;
	glm::vec3 topColor() const;
	NodeMarker* top();
	void skim();
protected:
	std::vector<NodeMarker> m_nodeData;
};