#include "Drop.h"
#include <iostream>
#include <stack>

Drop::Drop(glm::vec2 pos) 
{ 
    m_pos = pos; 
}

Drop::Drop(glm::vec2 pos, float volume) 
{
    m_pos = pos;
    m_volume = volume;
}

void Drop::cascade(glm::vec2 pos, glm::ivec2 dim, Node* nodes, bool* track, float& maxHeight)
{
    int ind = floor(pos.y) * dim.x + floor(pos.x);
    // stokes' law, see write-up
    float deposit = m_sedimentAmount * glm::max(0.1f, glm::min(0.5f, 1.257f * glm::max(1.0f, (float)m_velocity.length())));
    float depositCap = 0.02f;
    if (deposit > depositCap)
        deposit = depositCap;

    // neighbors
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
        //if (nodes[offsetIndex].waterDepth() > 0.1) 
        //   continue;

        float diff = glm::min(abs(nodes[ind].topHeight() - nodes[offsetIndex].topHeight()), 1.0f);

        // assuming verticality change. Really steep changes are capped, to prevent absurd force values that would be unsustainable IRL. (f=mv)
        float actingForce = m_volume * glm::max(glm::min(0.8f, (glm::distance(m_lastVelocity, m_velocity) + diff)), 0.2f);
        
        // very low velocity change! Likely that we're not really moving at all.
        if (actingForce <= 0.0f)
            continue;

        // van Rijn calculations for sediment transfer
        // cohesionless and size assumed to be similar to dirt/sand (30000 microns)
        float transportRate = pow(actingForce * pow((nodes[ind].top()->resistiveForce - 1) * 9.81f, 0.5f), 2.4f) * 0.0027507f;
        float transfer = actingForce / nodes[ind].top()->resistiveForce * transportRate;

        if (transfer >= 10.0f)
            std::cout << "ERROR: transfer really high? force error?";
        if (deposit >= 10.0f)
            std::cout << "ERROR: deposit really high? force error?";

        m_sedimentAmount = glm::min(10.0f, m_sedimentAmount + transfer);
        m_sediment.mix(nodes[ind].getDataAboveHeight(nodes[ind].topHeight() - transfer), glm::max(0.0f, glm::min(1.0f, transfer / m_sedimentAmount)));

        nodes[ind].setHeight(nodes[ind].topHeight() - transfer, m_sediment, maxHeight);

        m_sedimentAmount -= deposit;

        if(!nodes[offsetIndex].hasWater())
            nodes[offsetIndex].setHeight(nodes[offsetIndex].topHeight() + deposit, m_sediment, maxHeight);
    }
}

bool Drop::descend(glm::vec3 norm, Node* nodes, bool* track, glm::ivec2 dim, float& maxHeight) 
{
    // simulate behavior as a particle running down the landscape
    if (m_terminated)
        return false;

    if (m_volume < m_minVol)
        return false;

    m_lastVelocity = m_velocity;
    int index = (int)m_pos.y * dim.x + (int)m_pos.x;

    if (index < 0 || index >= dim.x * dim.y)
        return false;

    if (nodes[index].hasWater())
        m_terminated = true;

    nodes[index].setParticles(nodes[index].getParticles() + m_volume);

    // likely to flow into other water
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
    // frictional forces from foliage density. F=ma & f=μn
    float frictionCoefficient = glm::min(glm::max(0.1f, nodes[index].getFoliageDensity()), 0.7f);
    //scale to m/s, apply g
    float frictionalDecelleraion = frictionCoefficient * 0.981f * sin(theta);
    m_velocity -= m_velocity * glm::min(0.8f, frictionalDecelleraion);

    // more likely to travel to a location with water
    if (particleEffect != glm::vec2(0.0f))
    {
        particleEffect = glm::normalize(particleEffect);
        m_velocity += particleEffect * 0.05f;
    }

    // accelleration due to gravity, a=gSin(θ)
    // a is in m/s and needs to be scaled due to the extended time period (a year divided by our simulation steps)
    glm::vec2 a = glm::vec2(norm.x, norm.y) * sin(theta);
    m_velocity += a * 2628.0f;

    // barely moving- flat surface and no speed?
    if (glm::length(m_velocity) < 0.075f)
        return false;

    // we need to visit every possible square, so normalize velocity only for movement. This won't matter as we immediately simulate again and will keep moving!
    m_pos += glm::normalize(m_velocity) * (float)sqrt(2);
    int newIndex = (int)m_pos.y * dim.x + (int)m_pos.x;

    if (newIndex = m_prevIndex)
        m_retreadCount++;
    
    if (m_retreadCount > 500)
        return false;

    if (m_pos.x < 0 || m_pos.x >= dim.x || m_pos.y < 0 || m_pos.y >= dim.y)
        return false;

    m_volume *= 0.985f;
    m_sedimentAmount *= 0.985f;
    m_age++;
    m_prevIndex = index;
    cascade(m_pos, dim, nodes, track, maxHeight);
    return true;
}

bool Drop::flood(Node* nodes, glm::ivec2 dim) 
{
    float increaseAmount = 0.01f;
    while (m_volume > m_minVol)
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
            vol += plane - nodes[i].waterHeight();

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

        if (!set.empty() && currVolume != 0.0f && currVolume < m_volume)
        {
#ifdef WATERDEBUG
            std::cout << "flooding set of " << set.size() << " nodes at " << m_pos.x << ", " << m_pos.y << " to height " << plane << std::endl;
#endif // WATERDEBUG
            m_volume -= currVolume;

            for (int s : set)
            {
                nodes[s].setWaterHeight(plane);
            }
        }
        else if(set.size() > 1)
        {
            if (increaseAmount >= 0.0001f)
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
                    nodes[drain].erodeByValue(0.005f);
                    break;
                }

                for (int s : set)
                {
                    nodes[s].setWaterHeight(plane);
                }
                m_pos = drainPos;
                m_terminated = false;
            }
            else
            {
                // we're going to fill off the map, so "evaporate" and vanish
                m_volume = 0.0f;
            }
#ifdef WATERDEBUG
            std::cout << "overflowing particle from set of " << set.size() << "nodes at " << m_pos.x << ", " << m_pos.y << ". plane = " << plane << " drain = " << drainHeight << std::endl;
#endif // WATERDEBUG
        }
        else
        {
            // evaporate as nothing else can happen here- we can't fill anything at all
            m_volume = 0.0f;
        }

        delete[] tried;
    }
    return false;
}