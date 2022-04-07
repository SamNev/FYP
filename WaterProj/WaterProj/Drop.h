#include <functional>
#include <GL/glew.h>
#include <glm.hpp>
#include <unordered_map>

class Node;

class Drop {
public:
    Drop(glm::vec2 pos);
    Drop(glm::vec2 p, glm::ivec2 dim, float v);

    bool descend(glm::vec3 norm, Node* nodes, std::vector<float>* track, glm::ivec2 dim, float scale);
    bool flood(Node* nodes, glm::ivec2 dim);
    static void cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes);
    glm::vec2 getPosition() { return m_pos; }
    float getVolume() { return m_volume; }
    float getMinVolume() { return m_minVol; }

protected:
    int m_age = 0;
    glm::vec2 m_pos;
    glm::vec2 m_speed = glm::vec2(0.0);
    //1.0
    float m_volume = 100.0; 
    float m_sediment = 0.0;

    const float m_density = 1.0;
    const float m_evapRate = 0.001;
    const float m_depositionRate = 1.2 * 0.08;
    const float m_minVol = 0.01;
    const float m_friction = 0.25;
    const float m_volumeFactor = 0.5; 

    int m_remainingSpills = 0;

};