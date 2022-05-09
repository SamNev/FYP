#include <functional>
#include <glm.hpp>
#include <queue>
#include <unordered_map>

#include "Node.h"

// resolves conflict between GLM and windows defines
#define NOMINMAX
//#define WATERDEBUG

class MapParams;

class Drop {
public:
    Drop(glm::vec2 pos, MapParams* params);
    Drop(glm::vec2 p, float v, MapParams* params);

    bool descend(glm::vec3 norm, Node* nodes, bool* track, glm::ivec2 dim, float& maxHeight);
    bool flood(Node* nodes, glm::ivec2 dim);
    void cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes, bool* track, float& maxHeight);
    glm::vec2 getPosition() { return m_pos; }
    float getVolume() { return m_volume; }
    float getMinVolume();
    int getAge() { return m_age; }

protected:
    int m_age = 0;
    glm::vec2 m_pos;
    glm::vec2 m_velocity = glm::vec2(0.0);
    glm::vec2 m_lastVelocity = glm::vec2(0.0);
    std::queue<glm::vec2> m_previous;

    float m_volume = 1; 
    int m_prevIndex = 0;
    int m_retreadCount = 0;
    float m_sedimentAmount = 0.0f;
    NodeMarker m_sediment;
    MapParams* m_params;

    bool m_terminated = false;
};