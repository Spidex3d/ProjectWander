#include "World/TerrainManager.h"
#include <iostream>
#include <cmath>


bool TerrainManager::Initialize()
{
    Destroy();

    // Generate initial terrain around origin
    Update(glm::vec3(0.0f));

    const int expectedChunks =
        (m_worldHalfWidth * 2 + 1) *
        (m_streamDistance * 2 + 1);

    return m_chunks.size() ==
        static_cast<size_t>(expectedChunks);
}

//bool TerrainManager::Initialize()
//{
//    Destroy();
//
//    // Create the initial chunks around world origin
//    Update(glm::vec3(0.0f));
//
//    return m_chunks.size() == 9;
//}

void TerrainManager::Update(
    const glm::vec3& cameraPosition
)
{
    const float chunkSize =
        m_chunkResolution * m_chunkSpacing;

    // =====================================
    // Find camera chunk
    // =====================================

    int cameraChunkX = static_cast<int>(
        std::floor(cameraPosition.x / chunkSize)
        );

    int cameraChunkZ = static_cast<int>(
        std::floor(cameraPosition.z / chunkSize)
        );

    m_currentChunkX = cameraChunkX;
    m_currentChunkZ = cameraChunkZ;

    // =====================================
    // Generate required chunks
    // =====================================

    // Fixed X width
    for (int x = -m_worldHalfWidth;
        x <= m_worldHalfWidth;
        ++x)
    {
        // Dynamic Z range
        for (int z = cameraChunkZ - m_streamDistance;
            z <= cameraChunkZ + m_streamDistance;
            ++z)
        {
            ChunkKey key(x, z);

            // Skip existing chunks
            if (m_chunks.find(key) != m_chunks.end())
                continue;

            // Create a new chunk
            auto chunk =
                std::make_unique<TerrainChunk>(x, z);

            if (chunk->Generate(
                m_chunkResolution,
                m_chunkSpacing,
                m_settings))
            {
                m_chunks.emplace(
                    key,
                    std::move(chunk)
                );
            }
        }
    }

    // =====================================
    // Remove distant chunks
    // =====================================

    for (auto it = m_chunks.begin();
        it != m_chunks.end();)
    {
        int chunkX = it->first.first;
        int chunkZ = it->first.second;

        // Outside world width?
        bool outsideWidth =
            chunkX < -m_worldHalfWidth ||
            chunkX > m_worldHalfWidth;

        // Too far ahead or behind?
        bool outsideDistance =
            std::abs(chunkZ - cameraChunkZ) >
            m_streamDistance;

        if (outsideWidth || outsideDistance)
        {
            it = m_chunks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

//void TerrainManager::Update(const glm::vec3& cameraPosition)
//{
//    const float chunkSize =
//        m_chunkResolution * m_chunkSpacing;
//
//    // Calculate which chunk contains the camera
//    int cameraChunkX = static_cast<int>(
//        std::floor(cameraPosition.x / chunkSize)
//        );
//
//    int cameraChunkZ = static_cast<int>(
//        std::floor(cameraPosition.z / chunkSize)
//        );
//
//    m_currentChunkX = cameraChunkX;
//    m_currentChunkZ = cameraChunkZ;
//
//    // Generate missing chunks around the camera
//    for (int z = -m_viewDistance; z <= m_viewDistance; ++z)
//    {
//        for (int x = -m_viewDistance; x <= m_viewDistance; ++x)
//        {
//            int chunkX = cameraChunkX + x;
//            int chunkZ = cameraChunkZ + z;
//
//            ChunkKey key(chunkX, chunkZ);
//
//            // Only generate chunks that don't exist
//            if (m_chunks.find(key) == m_chunks.end())
//            {
//                auto chunk =
//                    std::make_unique<TerrainChunk>(
//                        chunkX,
//                        chunkZ
//                    );
//
//                if (chunk->Generate(
//                    m_chunkResolution,
//                    m_chunkSpacing,
//                    m_settings))
//                {
//                    m_chunks.emplace(
//                        key,
//                        std::move(chunk)
//                    );
//                }
//            }
//        }
//    }
//
//    // Remove chunks outside our view distance
//    for (auto it = m_chunks.begin(); it != m_chunks.end();)
//    {
//        int dx = std::abs(it->first.first - cameraChunkX);
//        int dz = std::abs(it->first.second - cameraChunkZ);
//
//        if (dx > m_viewDistance || dz > m_viewDistance)
//        {
//            it = m_chunks.erase(it);
//        }
//        else
//        {
//            ++it;
//        }
//    }
//}


void TerrainManager::Regenerate(
    const TerrainSettings& settings,
    const glm::vec3& cameraPosition)
{
    // Store the new generation settings
    m_settings = settings;

    // Delete old terrain chunks
    Destroy();

    // Generate chunks around the current camera
    Update(cameraPosition);
}


bool TerrainManager::GetHeightAt(
    float worldX,
    float worldZ,
    float& height
) const
{
    float chunkSize =
        m_chunkResolution * m_chunkSpacing;

    // Find the chunk containing this position
    int chunkX = static_cast<int>(
        std::floor(worldX / chunkSize)
        );

    int chunkZ = static_cast<int>(
        std::floor(worldZ / chunkSize)
        );

    ChunkKey key(chunkX, chunkZ);

    auto it = m_chunks.find(key);

    if (it == m_chunks.end())
        return false;

    return it->second->GetHeightAt(
        worldX,
        worldZ,
        height
    );
}

bool TerrainManager::SaveChunk(int chunkX, int chunkZ, const std::string& filename) const
{
    ChunkKey key(chunkX, chunkZ);

    auto it = m_chunks.find(key);

    if (it == m_chunks.end())
        return false;

    return it->second->SaveHeightmap(filename);
}

bool TerrainManager::LoadChunk(int chunkX, int chunkZ, const std::string& filename)
{
    ChunkKey key(chunkX, chunkZ);

    auto it = m_chunks.find(key);

    if (it == m_chunks.end())
        return false;

    return it->second->LoadHeightmap(filename);
}

bool TerrainManager::SaveChunkHeightmap(int chunkX, int chunkZ, const std::string& filename) const
{
    ChunkKey key(chunkX, chunkZ);

    auto it = m_chunks.find(key);

    if (it == m_chunks.end())
    {
        std::cout << "Chunk not loaded: "
            << chunkX << ", "
            << chunkZ << std::endl;

        return false;
    }

    return it->second->SaveHeightmapPNG(filename);
}

bool TerrainManager::LoadChunkHeightmap(
    int chunkX,
    int chunkZ,
    const std::string& filename
)
{
    ChunkKey key(chunkX, chunkZ);

    auto it = m_chunks.find(key);

    if (it == m_chunks.end())
    {
        std::cout << "Chunk not loaded: "
            << chunkX << ", "
            << chunkZ << '\n';

        return false;
    }

    return it->second->LoadHeightmapPNG(filename);
}
void TerrainManager::Render(
    const Shader& shader,
    const glm::mat4& view,
    const glm::mat4& projection
) const
{
    for (const auto& [position, chunk] : m_chunks)
    {
        chunk->Render(
            shader,
            view,
            projection
        );
    }
}

void TerrainManager::Destroy()
{
    m_chunks.clear();
}
//
//bool TerrainManager::Initialize()
//{
//    Destroy();
//
//    // Generate a 3 x 3 grid of chunks
//    for (int z = -1; z <= 1; ++z)
//    {
//        for (int x = -1; x <= 1; ++x)
//        {
//            auto chunk =
//                std::make_unique<TerrainChunk>(x, z);
//
//            if (!chunk->Generate(
//                m_chunkResolution,
//                m_chunkSpacing))
//            {
//                Destroy();
//                return false;
//            }
//
//            m_chunks.push_back(std::move(chunk));
//
//            std::cout << "Generated chunk: "
//                << x << ", " << z
//                << std::endl;
//        }
//    }
//
//    return true;
//}
//
//void TerrainManager::Render(
//    const Shader& shader,
//    const glm::mat4& view,
//    const glm::mat4& projection
//) const
//{
//    for (const auto& chunk : m_chunks)
//    {
//        chunk->Render(
//            shader,
//            view,
//            projection
//        );
//    }
//}
//
//void TerrainManager::Destroy()
//{
//    m_chunks.clear();
//}