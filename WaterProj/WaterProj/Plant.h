#pragma once
#include <glm.hpp>

//temporary
#define WSIZE 1000

class Node;

class Plant {

    Plant(int i, glm::ivec2 d);
    Plant(glm::vec2 p, glm::ivec2 d);

    glm::vec2 m_pos;
    int m_index;
    float m_size = 0.5;
    const float m_maxSize = 1.0;
    const float m_rate = 0.05;

    void grow();
    void root(Node* nodes, glm::ivec2 dim, float factor);

    Plant& operator=(const Plant& o) {
        if (this != &o) { 
            m_pos = o.m_pos;
            m_index = o.m_index;
            m_size = o.m_size;
        }
        return *this;
    };
};