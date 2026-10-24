#pragma once

#include "World/Terrain.h"

#include <glm/glm.hpp>
#include <string>

class Shader;

class TerrainChunk
{
public:

    TerrainChunk(int chunkX, int chunkZ);

    //bool Generate(int size, float spacing);
    bool Generate(int size, float spacing, const TerrainSettings& settings);

    bool GetHeightAt(float worldX, float worldZ, float& height) const;

    bool SaveHeightmap(const std::string& filename) const;
    bool LoadHeightmap(const std::string& filename);

    bool SaveHeightmapPNG(const std::string& filename) const;
    bool LoadHeightmapPNG(const std::string& filename);

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
