#pragma once

#include "World/TerrainChunk.h"
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <utility>

class Shader;

class TerrainManager
{
public:
    bool Initialize();

    // Update chunks based on camera position
    void Update(const glm::vec3& cameraPosition);

    bool GetHeightAt(float worldX, float worldZ, float& height) const;

    void Render(
        const Shader& shader,
        const glm::mat4& view,
        const glm::mat4& projection
    ) const;

    void Destroy();

    size_t GetChunkCount() const
    {
        return m_chunks.size();
    }

private:
    // Chunk coordinates: X, Z
    using ChunkKey = std::pair<int, int>;

    std::map<ChunkKey, std::unique_ptr<TerrainChunk>> m_chunks;

    int m_chunkResolution = 128;
    float m_chunkSpacing = 2.0f;

    // Number of chunks around the camera
    int m_viewDistance = 1;

    int m_currentChunkX = 0;
    int m_currentChunkZ = 0;
};

//
//#include <memory>
//#include <vector>
//
//class Shader;
//
//class TerrainManager
//{
//public:
//
//    bool Initialize();
//
//    void Render(
//        const Shader& shader,
//        const glm::mat4& view,
//        const glm::mat4& projection
//    ) const;
//
//    void Destroy();
//
//private:
//
//    std::vector<std::unique_ptr<TerrainChunk>> m_chunks;
//
//    int m_chunkResolution = 128;
//    float m_chunkSpacing = 2.0f;
//
//   // int m_chunkResolution = 64;
//    //float m_chunkSpacing = 2.0f;
//};