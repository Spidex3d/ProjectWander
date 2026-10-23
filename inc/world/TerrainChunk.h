#pragma once

#include "World/Terrain.h"

#include <glm/glm.hpp>

class Shader;

class TerrainChunk
{
public:

    TerrainChunk(int chunkX, int chunkZ);

    bool Generate(int size, float spacing);

    void Render(
        const Shader& shader,
        const glm::mat4& view,
        const glm::mat4& projection
    ) const;

    glm::ivec2 GetChunkPosition() const;

private:

    glm::ivec2 m_chunkPosition;

    Terrain m_terrain;
};
