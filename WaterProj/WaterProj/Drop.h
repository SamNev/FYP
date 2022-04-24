#include <functional>
#include <GL/glew.h>
#include <glm.hpp>
#include <unordered_map>

#include "Node.h"

//#define WATERDEBUG

class Drop {
public:
    Drop(glm::vec2 pos);
    Drop(glm::vec2 p, glm::ivec2 dim, float v);

    bool descend(glm::vec3 norm, Node* nodes, std::vector<bool>* track, glm::ivec2 dim, float scale);
    bool flood(Node* nodes, glm::ivec2 dim);
    void cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes, std::vector<bool>* track);
    glm::vec2 getPosition() { return m_pos; }
    float getVolume() { return m_volume; }
    float getMinVolume() { return m_minVol; }

protected:
    int m_age = 0;
    glm::vec2 m_pos;
    glm::vec2 m_velocity = glm::vec2(0.0);
    glm::vec2 m_lastVelocity = glm::vec2(0.0);
    //1.0
    float m_volume = 1; 

    const float m_density = 1.0;
    const float m_evapRate = 0.001;
    const float m_depositionRate = 1.2 * 0.08;
    const float m_minVol = 0.01;
    const float m_friction = 0.25;
    //0.5
    const float m_volumeFactor = 0.5; 

    float m_sedimentAmount = 0.0f;
    NodeMarker m_sediment;

    int m_remainingSpills = 0;

    int m_prevIndex;

};