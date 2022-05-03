#include <functional>
#include <GL/glew.h>
#include <glm.hpp>
#include <unordered_map>

#include "Node.h"

//#define WATERDEBUG

class Drop {
public:
    Drop(glm::vec2 pos);
    Drop(glm::vec2 p, float v);

    bool descend(glm::vec3 norm, Node* nodes, bool* track, glm::ivec2 dim, float& maxHeight);
    bool flood(Node* nodes, glm::ivec2 dim);
    void cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes, bool* track, float& maxHeight);
    glm::vec2 getPosition() { return m_pos; }
    float getVolume() { return m_volume; }
    float getMinVolume() { return m_minVol; }

protected:
    int m_age = 0;
    glm::vec2 m_pos;
    glm::vec2 m_velocity = glm::vec2(0.0);
    glm::vec2 m_lastVelocity = glm::vec2(0.0);

    float m_volume = 1; 
    int m_prevIndex = 0;
    int m_retreadCount = 0;
    const float m_minVol = 0.001;
    float m_sedimentAmount = 0.0f;
    NodeMarker m_sediment;

    bool m_terminated = false;
};