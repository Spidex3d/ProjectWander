#include "World/TerrainChunk.h"
#include "graphics/Shader.h"

TerrainChunk::TerrainChunk(int chunkX, int chunkZ)
    : m_chunkPosition(chunkX, chunkZ)
{}

bool TerrainChunk::Generate(
    int size,
    float spacing,
    const TerrainSettings& settings
)
{
    float chunkSize = size * spacing;

    glm::vec2 worldOrigin(
        m_chunkPosition.x * chunkSize,
        m_chunkPosition.y * chunkSize
    );

    return m_terrain.Generate(
        size,
        spacing,
        worldOrigin,
        settings
    );
}


bool TerrainChunk::GetHeightAt(
    float worldX,
    float worldZ,
    float& height
) const
{
    return m_terrain.GetHeightAt(
        worldX,
        worldZ,
        height
    );
}

void TerrainChunk::Render(
    const Shader& shader,
    const glm::mat4& view,
    const glm::mat4& projection
) const
{
    // Alternate chunk colours
    bool alternate =
        ((m_chunkPosition.x + m_chunkPosition.y) % 2 == 0);

    glm::vec3 chunkColor = alternate
        ? glm::vec3(0.25f, 0.65f, 0.25f)
        : glm::vec3(0.15f, 0.40f, 0.15f);

    shader.Use();
    shader.setVec3("uChunkColor", chunkColor);

    m_terrain.Render(shader, view, projection);
}


glm::ivec2 TerrainChunk::GetChunkPosition() const
{
    return m_chunkPosition;
}