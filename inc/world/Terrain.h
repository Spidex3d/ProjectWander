#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "World/TerrainSettings.h"


struct TerrainVertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

class Shader;

class Terrain
{
public:
    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;

    Terrain() = default;
    ~Terrain();

    bool Generate(int size, float spacing, const glm::vec2& chunkOrigin = glm::vec2(0.0f), const TerrainSettings& settings = TerrainSettings());


    bool GetHeightAt(float worldX, float worldZ, float& height) const;


    void Render(
        const Shader& shader,
        const glm::mat4& view,
        const glm::mat4& projection
    ) const;

    void Destroy();

private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_EBO = 0;

    //std::vector<glm::vec3> m_vertices;
    std::vector<TerrainVertex> m_vertices;
    std::vector<unsigned int> m_indices;

    int m_gridSize = 0;
    float m_gridSpacing = 1.0f;
    glm::vec2 m_chunkOrigin = glm::vec2(0.0f);

    int m_indexCount = 0;
};