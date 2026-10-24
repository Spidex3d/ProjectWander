#pragma once

#include "World/TerrainChunk.h"
#include "World/TerrainSettings.h"
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <utility>
#include <string>

class Shader;

class TerrainManager
{
public:
    bool Initialize();

    // Update chunks based on camera position
    void Update(const glm::vec3& cameraPosition);

    bool GetHeightAt(float worldX, float worldZ, float& height) const;

    bool SaveChunk(int chunkX, int chunkZ, const std::string& filename) const;
    bool LoadChunk(int chunkX, int chunkZ, const std::string& filename);

    bool SaveChunkHeightmap(int chunkX, int chunkZ, const std::string& filename) const;
    bool LoadChunkHeightmap(int chunkX, int chunkZ, const std::string& filename);

    void Render(
        const Shader& shader,
        const glm::mat4& view,
        const glm::mat4& projection
    ) const;

    // Access current terrain settings
    const TerrainSettings& GetSettings() const
    {
        return m_settings;
    }

    // Apply new settings and regenerate loaded terrain
    void Regenerate(const TerrainSettings& settings, const glm::vec3& cameraPosition);

    void Destroy();

    size_t GetChunkCount() const
    {
        return m_chunks.size();
    }

private:
    // Chunk coordinates: X, Z
    using ChunkKey = std::pair<int, int>;

    std::map<ChunkKey, std::unique_ptr<TerrainChunk>> m_chunks;
    
    TerrainSettings m_settings;
    
    int m_chunkResolution = 128;
    float m_chunkSpacing = 2.0f;

    // =====================================
    // World dimensions
    // =====================================

   // Fixed world width:
   // -2, -1, 0, +1, +2
    int m_worldHalfWidth = 2;

    // Number of chunk rows ahead and behind camera
    int m_streamDistance = 2;

    int m_currentChunkX = 0;
    int m_currentChunkZ = 0;
};

