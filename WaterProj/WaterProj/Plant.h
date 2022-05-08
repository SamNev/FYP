#pragma once
#include <glm.hpp>

class Node;

class Plant {
public:
    static void root(Node* nodes, glm::ivec2 dim, glm::ivec2 pos, float factor);
    static float getFertilityForNode(const Node* node);
};