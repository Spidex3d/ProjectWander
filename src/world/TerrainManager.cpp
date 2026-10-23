#include "World/TerrainManager.h"
#include <iostream>
#include <cmath>

bool TerrainManager::Initialize()
{
    Destroy();

    // Create the initial chunks around world origin
    Update(glm::vec3(0.0f));

    return m_chunks.size() == 9;
}

void TerrainManager::Update(const glm::vec3& cameraPosition)
{
    const float chunkSize =
        m_chunkResolution * m_chunkSpacing;

    // Calculate which chunk contains the camera
    int cameraChunkX = static_cast<int>(
        std::floor(cameraPosition.x / chunkSize)
        );

    int cameraChunkZ = static_cast<int>(
        std::floor(cameraPosition.z / chunkSize)
        );

    m_currentChunkX = cameraChunkX;
    m_currentChunkZ = cameraChunkZ;

    // Generate missing chunks around the camera
    for (int z = -m_viewDistance; z <= m_viewDistance; ++z)
    {
        for (int x = -m_viewDistance; x <= m_viewDistance; ++x)
        {
            int chunkX = cameraChunkX + x;
            int chunkZ = cameraChunkZ + z;

            ChunkKey key(chunkX, chunkZ);

            // Only generate chunks that don't exist
            if (m_chunks.find(key) == m_chunks.end())
            {
                auto chunk =
                    std::make_unique<TerrainChunk>(
                        chunkX,
                        chunkZ
                    );

                if (chunk->Generate(
                    m_chunkResolution,
                    m_chunkSpacing))
                {
                    m_chunks.emplace(
                        key,
                        std::move(chunk)
                    );
                }
            }
        }
    }

    // Remove chunks outside our view distance
    for (auto it = m_chunks.begin(); it != m_chunks.end();)
    {
        int dx = std::abs(it->first.first - cameraChunkX);
        int dz = std::abs(it->first.second - cameraChunkZ);

        if (dx > m_viewDistance || dz > m_viewDistance)
        {
            it = m_chunks.erase(it);
        }
        else
        {
            ++it;
        }
    }
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