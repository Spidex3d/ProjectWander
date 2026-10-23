#include "World/Terrain.h"
#include "graphics/Shader.h"
#include <cmath>
#include <cstddef> // offsetof
#include <glm/gtc/noise.hpp>

namespace
{
    float GetTerrainHeight(
        float x,
        float z,
        float heightScale,
        float noiseFrequency,
        int octaves)
    {
        float height = 0.0f;
        float amplitude = 1.0f;
        float frequency = noiseFrequency;

        for (int i = 0; i < octaves; ++i)
        {
            float noise = glm::perlin(
                glm::vec2(x * frequency, z * frequency)
            );

            height += noise * amplitude;

            amplitude *= 0.5f;
            frequency *= 2.0f;
        }

        return height * heightScale;
    }
}



Terrain::~Terrain()
{
    Destroy();
}

bool Terrain::Generate(int size, float spacing, const glm::vec2& chunkOrigin)
{
    // Remove any previously generated terrain
    Destroy();

    m_vertices.clear();
    m_indices.clear();

    if (size <= 0 || spacing <= 0.0f)
        return false;

    m_gridSize = size;
    m_gridSpacing = spacing;
    m_chunkOrigin = chunkOrigin;
    
    // Generate terrain vertices
    for (int z = 0; z <= size; ++z)
    {
        for (int x = 0; x <= size; ++x)
        {
            // World position
            float xPos = chunkOrigin.x + x * spacing;
            float zPos = chunkOrigin.y + z * spacing;

            // Terrain height
            float height = GetTerrainHeight(
                xPos,
                zPos,
                HeightScale,
                NoiseFrequency,
                NoiseOctaves
            );

            // Sample neighbouring heights
            float hL = GetTerrainHeight(
                xPos - spacing, zPos,
                HeightScale, NoiseFrequency, NoiseOctaves
            );

            float hR = GetTerrainHeight(
                xPos + spacing, zPos,
                HeightScale, NoiseFrequency, NoiseOctaves
            );

            float hD = GetTerrainHeight(
                xPos, zPos - spacing,
                HeightScale, NoiseFrequency, NoiseOctaves
            );

            float hU = GetTerrainHeight(
                xPos, zPos + spacing,
                HeightScale, NoiseFrequency, NoiseOctaves
            );

            // Calculate normal
            glm::vec3 normal = glm::normalize(
                glm::vec3(
                    hL - hR,
                    2.0f * spacing,
                    hD - hU
                )
            );

            // Texture coordinates
            glm::vec2 texCoord(
                xPos * 0.1f,
                zPos * 0.1f
            );

            TerrainVertex vertex;

            vertex.Position = glm::vec3(
                xPos,
                height,
                zPos
            );

            vertex.Normal = normal;
            vertex.TexCoord = texCoord;

            m_vertices.push_back(vertex);
        }
    }
    
    // Generate triangle indices
    for (int z = 0; z < size; ++z)
    {
        for (int x = 0; x < size; ++x)
        {
            unsigned int topLeft =
                z * (size + 1) + x;

            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * (size + 1) + x;
            unsigned int bottomRight = bottomLeft + 1;

            // First triangle
            m_indices.push_back(topLeft);
            m_indices.push_back(bottomLeft);
            m_indices.push_back(topRight);

            // Second triangle
            m_indices.push_back(topRight);
            m_indices.push_back(bottomLeft);
            m_indices.push_back(bottomRight);
        }
    }

    m_indexCount = static_cast<int>(m_indices.size());

    // Create OpenGL buffers
    // Create OpenGL buffers
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    // ====================================
    // Upload vertex data
    // ====================================

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        m_vertices.size() * sizeof(TerrainVertex),
        m_vertices.data(),
        GL_STATIC_DRAW
    );

    // ====================================
    // Upload triangle indices
    // ====================================

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        m_indices.size() * sizeof(unsigned int),
        m_indices.data(),
        GL_STATIC_DRAW
    );

    // ====================================
    // Vertex attributes
    // ====================================

    // Position - location 0
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TerrainVertex),
        (void*)offsetof(TerrainVertex, Position)
    );

    glEnableVertexAttribArray(0);

    // Normal - location 1
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TerrainVertex),
        (void*)offsetof(TerrainVertex, Normal)
    );

    glEnableVertexAttribArray(1);

    // Texture coordinates - location 2
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TerrainVertex),
        (void*)offsetof(TerrainVertex, TexCoord)
    );

    glEnableVertexAttribArray(2);

    // Unbind VAO
    glBindVertexArray(0);
    return true;
}

bool Terrain::GetHeightAt(
    float worldX,
    float worldZ,
    float& height
) const
{
    if (m_vertices.empty() || m_gridSize <= 0)
        return false;

    // Convert world coordinates to local grid coordinates
    float gridX =
        (worldX - m_chunkOrigin.x) / m_gridSpacing;

    float gridZ =
        (worldZ - m_chunkOrigin.y) / m_gridSpacing;

    // Check whether the position is inside this chunk
    if (gridX < 0.0f || gridZ < 0.0f ||
        gridX >= m_gridSize || gridZ >= m_gridSize)
    {
        return false;
    }

    int x = static_cast<int>(std::floor(gridX));
    int z = static_cast<int>(std::floor(gridZ));

    float fx = gridX - x;
    float fz = gridZ - z;

    int rowSize = m_gridSize + 1;

    // Four corner heights
    float h00 = m_vertices[
        z * rowSize + x
    ].Position.y;

    float h10 = m_vertices[
        z * rowSize + x + 1
    ].Position.y;

    float h01 = m_vertices[
        (z + 1) * rowSize + x
    ].Position.y;

    float h11 = m_vertices[
        (z + 1) * rowSize + x + 1
    ].Position.y;

    // Interpolate using the same triangles
    // as our terrain index buffer
    if (fx + fz <= 1.0f)
    {
        height =
            h00 +
            (h10 - h00) * fx +
            (h01 - h00) * fz;
    }
    else
    {
        height =
            h11 +
            (h01 - h11) * (1.0f - fx) +
            (h10 - h11) * (1.0f - fz);
    }

    return true;
}



void Terrain::Render(
    const Shader& shader,
    const glm::mat4& view,
    const glm::mat4& projection
) const
{

    if (m_VAO == 0 || m_indexCount == 0)
        return;

    
    shader.Use();

    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    glm::mat4 model(1.0f);
    shader.setMat4("model", model);

    glBindVertexArray(m_VAO);

    glDrawElements(
        GL_TRIANGLES,
        m_indexCount,
        GL_UNSIGNED_INT,
        nullptr
    );

    glBindVertexArray(0);
}

void Terrain::Destroy()
{
    if (m_EBO != 0)
    {
        glDeleteBuffers(1, &m_EBO);
        m_EBO = 0;
    }

    if (m_VBO != 0)
    {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }

    if (m_VAO != 0)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }

    m_indexCount = 0;
}