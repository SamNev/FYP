#include "Drop.h"

#include <iostream>
#include <stack>

#include "Map.h"

Drop::Drop(glm::vec2 pos, MapParams* params) 
{
    m_params = params;
    m_volume = params->dropDefaultVolume;
    m_pos = pos; 
}

Drop::Drop(glm::vec2 pos, float volume, MapParams* params) 
{
    m_params = params;
    m_pos = pos;
    m_volume = volume;
}

void Drop::cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes, bool* track, float& maxHeight)
{
    int ind = floor(pos.y) * dim.x + floor(pos.x);

    // Don't simulate transfer if we're stuck on one tile
    if (m_prevIndex == ind)
        return;

    // Avoid /0 and incredibly high deposit values due to sediment not moving at all
    if (m_velocity.length() < m_params->dropSedimentSimulationMinimumVelocity)
        return;

    // Stokes' law, see write-up. Particle density assumed at 1500kg/m^3
    float deposit = m_sedimentAmount * glm::max(0.1f, glm::min(0.5f, 2.4627f / (float)m_velocity.length()));
    deposit /= 8.0f;

    deposit = glm::min(m_params->dropSedimentDepositCap, deposit);

    // For each neighboring node
    const int nx[8] = { -1,-1,-1, 0, 0, 1, 1, 1 };
    const int ny[8] = { -1, 0, 1,-1, 1,-1, 0, 1 };

    for (int i = 0; i < 8; i++) 
    {
        glm::ivec2 offsetPos = (glm::ivec2)pos + glm::ivec2(nx[i], ny[i]);
        int offsetIndex = offsetPos.y * dim.x + offsetPos.x;

        if (offsetPos.x >= dim.x || offsetPos.y >= dim.y || offsetPos.x < 0 || offsetPos.y < 0)
            continue;

        if (offsetIndex == m_prevIndex)
            continue;

        track[offsetIndex] = true;

        float diff = glm::min(abs(nodes[ind].topHeight() - nodes[offsetIndex].topHeight()), 1.0f);
        float actingForce = m_volume * glm::max(glm::min(0.8f, (glm::distance(glm::normalize(m_lastVelocity), glm::normalize(m_velocity)))), 0.2f);

        if (m_lastVelocity == glm::vec2(0.0f))
            actingForce = 0.0f;

        // very low velocity change! Likely that we're not really moving at all.
        if (actingForce <= 0.0f)
            continue;

        // Van Rijn calculations for sediment transfer
        // Cohesionless and size assumed to be similar to dirt/sand (30000 microns)
        float transportRate = pow(actingForce * pow((nodes[ind].top()->resistiveForce - 1) * 0.02943f, -0.5f), 2.4f) * 0.0027507f;
        float transfer = actingForce * transportRate;
        // Modify based on height difference, to account for exposed amount of surface
        transfer *= glm::max(1.0f, (1.5f - diff));

        m_sedimentAmount = glm::min(m_params->dropContainedSedimentCap, m_sedimentAmount + transfer);
        m_sediment.mix(nodes[ind].getDataAboveHeight(nodes[ind].topHeight() - transfer), glm::max(0.0f, glm::min(1.0f, transfer / m_sedimentAmount)));

        nodes[ind].setHeight(nodes[ind].topHeight() - transfer, m_sediment, maxHeight);

        m_sedimentAmount -= deposit;

        if(!nodes[offsetIndex].hasWater())
            nodes[offsetIndex].setHeight(nodes[offsetIndex].topHeight() + deposit, m_sediment, maxHeight);
    }
}

bool Drop::descend(glm::vec3 norm, Node* nodes, bool* track, glm::ivec2 dim, float& maxHeight) 
{
    // Simulate behavior as a particle running down the landscape
    if (m_terminated)
        return false;

    if (m_volume < m_params->dropMinimumVolume)
        return false;

    m_lastVelocity = m_velocity;
    int index = (int)m_pos.y * dim.x + (int)m_pos.x;

    if (index < 0 || index >= dim.x * dim.y)
        return false;

    nodes[index].setParticles(nodes[index].getParticles() + m_volume);

    // Likely to flow into other water
    glm::vec2 particleEffect(0.0f);
    if(index - dim.x > 0)
        particleEffect.y -= nodes[index - dim.x].getParticles();
    if (index + dim.x < dim.x * dim.y)
        particleEffect.y += nodes[index + dim.x].getParticles();
    if(index - 1 > 0)
        particleEffect.x -= nodes[index - 1].getParticles();
    if (index + 1 < dim.x * dim.y)
        particleEffect.x += nodes[index + 1].getParticles();

    // θ can be found with dot product
    float theta = acos(glm::dot(norm, glm::vec3(0.0f, 1.0f, 0.0f)));
    // Frictional forces from foliage density. F=ma & f=μn
    float frictionCoefficient = glm::min(glm::max(0.1f, nodes[index].getFoliageDensity()), 0.7f);
    // Scale to m/s, apply g
    float frictionalDecelleraion = frictionCoefficient * 0.981f * sin(theta);
    m_velocity -= m_velocity * glm::min(0.8f, frictionalDecelleraion);

    // More likely to travel to a location with water
    if (particleEffect != glm::vec2(0.0f))
    {
        particleEffect = glm::normalize(particleEffect);
        m_velocity += particleEffect * m_params->particleSwayMagnitude;
    }

    // Accelleration due to gravity, a=gSin(θ)
    // a is in m/s and needs to be scaled due to the extended time period (a year divided by our simulation steps)
    glm::vec2 a = glm::vec2(norm.x, norm.y) * sin(theta);
    m_velocity += a * 26.28f;

    // Barely moving- flat surface and no speed?
    if (glm::length(m_velocity) < m_params->dropSedimentSimulationTerminationVelocity)
        return false;

    // We need to visit every possible square, so normalize velocity only for movement. This won't matter as we immediately simulate again and will keep moving!
    m_pos += glm::normalize(m_velocity) * (float)sqrt(2);

    if (m_pos.x < 0 || m_pos.x >= dim.x || m_pos.y < 0 || m_pos.y >= dim.y)
        return false;

    m_volume *= m_params->particleEvaporationRate;
    m_sedimentAmount *= m_params->particleEvaporationRate;
    m_age++;

    if (m_previous.size() >= 10)
    {
        glm::ivec2 prev = m_previous.front();

        if (distance(m_previous.front(), m_pos) < m_params->particleTerminationProximity || nodes[prev.y * dim.x + prev.x].hasWater())
            m_terminated = true;
       
        m_previous.pop();
    }

    m_previous.push(m_pos);

    cascade(m_pos, dim, nodes, track, maxHeight);
    m_prevIndex = index;
    return true;
}

bool Drop::flood(Node* nodes, glm::ivec2 dim, float& maxHeight) 
{
    float increaseAmount = m_params->floodDefaultIncrease;
    while (m_volume > m_params->dropMinimumVolume)
    {
        int index = (int)m_pos.y * dim.x + (int)m_pos.x;
        if (index < 0 || index >= dim.x * dim.y)
            return false;
        float plane = nodes[index].waterHeight() + increaseAmount;

        std::stack<int> toTry;
        std::vector<int> set;
        std::vector<int> border;
        const int size = (int)dim.x * dim.y;
        bool* tried = new bool[size];
        bool offMap = false;

        std::fill(tried, tried + size, false);

        std::function<bool(int)> inBounds = [&](int i)
        {
            if (i < 0 || i >= size)
            {
                offMap = true;
                return false;
            }

            if (tried[i])
                return false;

            tried[i] = true;

            return true;
        };

        std::function<void(int, float&)> fill = [&](int i, float& vol) 
        {
            if (plane < nodes[i].waterHeight()) {
                border.push_back(i);
                return;
            }

            set.push_back(i);
            vol += glm::max(0.0f, plane - nodes[i].waterHeight());

            if (inBounds(i + dim.x))
                toTry.push(i + dim.x);
            if (inBounds(i - dim.x))
                toTry.push(i - dim.x);
            if (inBounds(i + 1))
                toTry.push(i + 1);
            if (inBounds(i - 1))
                toTry.push(i - 1);
            if (inBounds(i + dim.x + 1))
                toTry.push(i + dim.x + 1);
            if (inBounds(i - dim.x - 1))
                toTry.push(i - dim.x - 1);
            if (inBounds(i + dim.x - 1))
                toTry.push(i + dim.x - 1);
            if (inBounds(i - dim.x + 1))
                toTry.push(i - dim.x + 1);
        };

        if (inBounds(index))
            toTry.push(index);
        else
            break;

        float currVolume = 0.0f;
        while (!toTry.empty())
        {
            if (currVolume > m_volume)
                break;

            int current = toTry.top();
            toTry.pop();
            fill(current, currVolume);
        }

        if (set.size() > 1 && currVolume != 0.0f && currVolume < m_volume)
        {
#ifdef WATERDEBUG
            std::cout << "flooding set of " << set.size() << " nodes at " << m_pos.x << ", " << m_pos.y << " to height " << plane << std::endl;
#endif // WATERDEBUG
            m_volume -= currVolume;

            transportThroughPool(nodes, dim, &set, maxHeight);
            for (int s : set)
            {
                nodes[s].setWaterHeight(plane);
            }
        }
        else if(set.size() > 1)
        {
            // Not a parameter, as changing this can really impact performance
            if (increaseAmount >= 0.00001f)
            {
                increaseAmount /= 10.0f;
                continue;
            }

            if (!nodes[index].hasWater())
                break;

            set.clear();
            border.clear();
            toTry.push(index); 
            std::fill(tried, tried + size, false);
            currVolume = 0.0f;
            offMap = false;
            plane = nodes[index].waterHeight();
            while (!toTry.empty())
            {
                int current = toTry.top();
                toTry.pop();
                fill(current, currVolume);
            }

            if (!offMap)
            {
                int drain = 0;
                float drainHeight = FLT_MAX;
                for (int potentialDrain : border)
                {
                    float height = nodes[drain].waterHeight();
                    if (height < drainHeight)
                    {
                        drain = potentialDrain;
                        drainHeight = height;
                    }
                }

                glm::vec2 drainPos = glm::vec2(drain % dim.x, drain / dim.x);
                if (m_pos == drainPos)
                {
                    nodes[drain].erodeByValue(m_params->drainErosionAmount);
                    break;
                }

                transportThroughPool(nodes, dim, &set, maxHeight);

                for (int s : set)
                {
                    nodes[s].setWaterHeight(plane);
                }

                m_pos = drainPos;
                m_terminated = false;
            }
            else
            {
                // We're going to fill off the map, so "evaporate" and vanish
                m_volume = 0.0f;
            }
#ifdef WATERDEBUG
            std::cout << "overflowing particle from set of " << set.size() << "nodes at " << m_pos.x << ", " << m_pos.y << ". plane = " << plane << " drain = " << drainHeight << std::endl;
#endif // WATERDEBUG
        }
        else
        {
            // Evaporate as nothing else can happen here- we can't fill anything at all
            m_volume = 0.0f;
        }

        delete[] tried;
    }
    return false;
}
#pragma optimize( "", off )
void Drop::transportThroughPool(Node* nodes, glm::vec2 dim, std::vector<int>* set, float& maxHeight)
{
#ifdef WATERDEBUG
    std::cout << "mixing sediment in pool of " << set->size() << std::endl;
#endif // WATERDEBUG
    if (!set || set->size() < 1)
        return;

    NodeMarker sediment = m_sediment;
    float sedimentAmount = m_sedimentAmount;
    float depth = 0.0f;
    for (int s : *set)
    {
        if (nodes[s].top()->resistiveForce < 10.0f)
        {
            float transfer = m_volume * pow(m_volume * pow((nodes[s].top()->resistiveForce - 1) * 0.02943f, -0.5f), 2.4f) * 0.0027507f;
            sedimentAmount += transfer;
            sediment.mix(nodes[s].getDataAboveHeight(nodes[s].topHeight() - transfer, true), transfer / sedimentAmount);
            nodes[s].erodeByValue(transfer);
        }

        depth += nodes[s].waterDepth();
    }

    sedimentAmount *= m_params->poolSedimentLossRate;
    if (depth == 0)
        return;

    for (int s : *set)
    {
        float transfer = (nodes[s].waterDepth() / depth) * sedimentAmount;
        nodes[s].setHeight(nodes[s].topHeight() + transfer, sediment, maxHeight);
    }
}
#pragma optimize( "", on )

float Drop::getMinVolume()
{
    return m_params->dropMinimumVolume;
}