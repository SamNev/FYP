#include <functional>
#include <glm.hpp>
#include <queue>
#include <unordered_map>

#include "Node.h"

// Resolves conflict between GLM and windows defines
#define NOMINMAX
//#define WATERDEBUG

class MapParams;

/***************************************************************************//**
 * Drop performs all fluid simulation calculations for the program.
 *
 * It will update terrain heights, manage the aquisition and deposition of
 * sediment, and form pools where acceptable. Each particle simulation is
 * independant of other particles, but their tracked movement may be used to
 * influence other particle simulation.
 ******************************************************************************/
class Drop {
public:
    /***************************************************************************//**
     * The drop constuctor establishes the position of the drop and provides reference
     * to the parameters of the current map.
     * @param pos The particle's current position
     * @param params The map parameters of the map this exists on
     ******************************************************************************/
    Drop(glm::vec2 pos, MapParams* params);
    /***************************************************************************//**
     * The drop constuctor establishes the position of the drop and provides reference
     * to the parameters of the current map.
     * @param pos The particle's current position
     * @param params The map parameters of the map this exists on
     * @param v The volume of the particle
     ******************************************************************************/
    Drop(glm::vec2 p, float v, MapParams* params);

    /***************************************************************************//**
     * Movement simulation for a single particle. Handles all the tracking and
     * gravitational calculations needed for simulation
     * @param norm The normal of the current tile
     * @param nodes Pointer to the node array that makes up the map
     * @param track A series of flags to allow particle movement to be tracked by the map
     * @param dim The dimesions of the map
     * @param maxHeight The maximum height of the map
     ******************************************************************************/
    bool descend(glm::vec3 norm, Node* nodes, bool* track, glm::ivec2 dim, float& maxHeight);
    /***************************************************************************//**
     * Flood simulation for a particle, attempting to join or create a pool
     * @param nodes Pointer to the node array that makes up the map
     * @param dim The dimesions of the map
     * @param maxHeight The maximum height of the map
     ******************************************************************************/
    bool flood(Node* nodes, glm::ivec2 dim, float& maxHeight);
    /***************************************************************************//**
     * Cascading for a particle, picking up and depositing sediment after a descent.
     * @param pos The position of the particle
     * @param dim The dimesions of the map
     * @param nodes Pointer to the node array that makes up the map
     * @param track A series of flags to allow particle movement to be tracked by the map
     * @param maxHeight The maximum height of the map
     ******************************************************************************/
    void cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes, bool* track, float& maxHeight);
    /***************************************************************************//**
     * Transporting sediment through a defined pool. This will evenly mix all top value
     * sediment within the given set, and deposit it accordingly.
     * @param nodes Pointer to the node array that makes up the map
     * @param dim The dimesions of the map
     * @param set A set of index values for nodes within the pool, to have their sediment mixed
     * @param maxHeight The maximum height of the map
     ******************************************************************************/
    void transportThroughPool(Node* nodes, glm::vec2 dim, std::vector<int>* set, float& maxHeight);

    glm::vec2 getPosition() { return m_pos; }
    float getVolume() { return m_volume; }
    int getAge() { return m_age; }
    float getMinVolume();

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