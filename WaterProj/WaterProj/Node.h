#pragma once

#include <GL/glew.h>
#include <glm.hpp>
#include <vector>

struct NodeMarker
{
	float height;
	float density;
};

class Node {
public:
	void addMarker(float height, float density);
	NodeMarker* top();
protected:
	std::vector<NodeMarker> m_nodeData;
};