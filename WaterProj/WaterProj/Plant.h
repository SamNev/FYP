#pragma once
#include <glm.hpp>

//temporary
#define WSIZE 1000

class Node;

class Plant {
public:
    Plant(int i, glm::ivec2 d);
    Plant(glm::vec2 p, glm::ivec2 d);

    Plant& operator=(const Plant& o) {
        if (this != &o) {
            m_pos = o.getPosition();
            m_index = o.getIndex();
            m_size = o.getSize();
        }
        return *this;
    };

    void grow();
    void root(Node* nodes, glm::ivec2 dim, float factor);
    glm::vec2 getPosition() const { return m_pos; } 
    int getIndex() const { return m_index; }
    float getSize() const { return m_size; }

protected:
    glm::vec2 m_pos;
    int m_index;
    float m_size = 0.5f;
    const float m_maxSize = 1.0f;
    const float m_rate = 0.05f;
};