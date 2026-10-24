#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "World/Terrain.h"
#include "graphics/Shader.h"

#include <cstddef> // offsetof
#include <glm/gtc/noise.hpp>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace
{
    float GetSeedOffset(int seed, unsigned int salt)
    {
        // Deterministic integer mixing
        unsigned int value =
            static_cast<unsigned int>(seed) ^ salt;

        value ^= value >> 16;
        value *= 0x7feb352dU;
        value ^= value >> 15;
        value *= 0x846ca68bU;
        value ^= value >> 16;

        // Return a stable offset between
        // approximately -10000 and +10000.
        return
            static_cast<float>(value % 20001U)
            - 10000.0f;
    }

    // Write a 32-bit value in little-endian order
    void WriteU32(std::ostream& out, uint32_t value)
    {
        for (int i = 0; i < 4; ++i)
        {
            out.put(static_cast<char>(
                (value >> (i * 8)) & 0xFF
                ));
        }
    }

    // Read a 32-bit little-endian value
    bool ReadU32(std::istream& in, uint32_t& value)
    {
        value = 0;

        for (int i = 0; i < 4; ++i)
        {
            int byte = in.get();

            if (byte == EOF)
                return false;

            value |=
                static_cast<uint32_t>(byte) << (i * 8);
        }

        return true;
    }

    // Write an IEEE-754 32-bit float
    void WriteF32(std::ostream& out, float value)
    {
        static_assert(sizeof(float) == sizeof(uint32_t));

        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));

        WriteU32(out, bits);
    }

    // Read an IEEE-754 32-bit float
    bool ReadF32(std::istream& in, float& value)
    {
        uint32_t bits;

        if (!ReadU32(in, bits))
            return false;

        std::memcpy(&value, &bits, sizeof(value));

        return true;
    }


    float GetTerrainHeight(float x, float z, const TerrainSettings& settings)
    {
        // ====================================
        // World seed offsets
       // ====================================

     // Hills
        float hillOffsetX = GetSeedOffset(
            settings.WorldSeed,
            0x1234U
        );

        float hillOffsetZ = GetSeedOffset(
            settings.WorldSeed,
            0x5678U
        );

        // Mountains
        float mountainOffsetX = GetSeedOffset(
            settings.WorldSeed,
            0x9ABCU
        );

        float mountainOffsetZ = GetSeedOffset(
            settings.WorldSeed,
            0xDEF0U
        );

        // Mountain distribution
        float maskOffsetX = GetSeedOffset(
            settings.WorldSeed,
            0x1357U
        );

        float maskOffsetZ = GetSeedOffset(
            settings.WorldSeed,
            0x2468U
        );


        // ====================================
        // Rolling hills
        // ====================================

        float hills = 0.0f;

        float amplitude = 1.0f;
        float frequency = settings.NoiseFrequency;

        for (int i = 0; i < settings.NoiseOctaves; ++i)
        {
            float noise = glm::perlin(
                glm::vec2(
                    x * frequency + hillOffsetX,
                    z * frequency + hillOffsetZ
                )
            );


            hills += noise * amplitude;

            amplitude *= 0.5f;
            frequency *= 2.0f;
        }

        hills *= settings.HillHeight;


        // ====================================
        // Mountain ridges
        // ====================================

        float mountains = 0.0f;

        amplitude = 1.0f;
        frequency = settings.NoiseFrequency * 0.5f;

        for (int i = 0; i < settings.NoiseOctaves; ++i)
        {
            float noise = glm::perlin(
                glm::vec2(
                    x * frequency + mountainOffsetX,
                    z * frequency + mountainOffsetZ
                )
            );


            float ridge = 1.0f - std::abs(noise);

            // Control ridge sharpness
            ridge = std::pow(
                glm::clamp(ridge, 0.0f, 1.0f),
                settings.RidgeSharpness
            );

            mountains += ridge * amplitude;

            amplitude *= 0.5f;
            frequency *= 2.0f;
        }


        // ====================================
        // Mountain distribution
        // ====================================

        float mountainMask = glm::perlin(
            glm::vec2(
                x * settings.MountainFrequency + maskOffsetX,
                z * settings.MountainFrequency + maskOffsetZ
            )
        );


        mountainMask =
            mountainMask * 0.5f + 0.5f;

        mountainMask = glm::smoothstep(
            settings.MountainStart,
            settings.MountainEnd,
            mountainMask
        );


        // ====================================
        // Final terrain height
        // ====================================

        // ====================================
    // Existing procedural terrain
    // ====================================

        float mountainHeight =
            mountains *
            settings.HillHeight *
            settings.MountainHeight;

        float terrainHeight =
            hills + mountainHeight * mountainMask;


        // ====================================
        // Valley flattening
        // ====================================

    // Centre of our five-column world
        constexpr float worldCentreX = 128.0f;

        // Distance from the valley centre
        float distanceFromCentre =
            std::abs(x - worldCentreX);

        // Calculate how strongly to flatten terrain
        float valleyBlend = 1.0f - glm::smoothstep(
            settings.ValleyWidth,
            settings.ValleyBlendEnd,
            distanceFromCentre
        );

        // Apply valley flatness
        float flattenAmount =
            valleyBlend * settings.ValleyFlatness;

        // Keep a small amount of gentle elevation variation
        float valleyFloor = glm::perlin(
            glm::vec2(
                x * 0.002f + hillOffsetX,
                z * 0.002f + hillOffsetZ
            )
        ) * 3.0f;

        // Blend original terrain towards the valley floor
        terrainHeight = glm::mix(
            terrainHeight,
            valleyFloor,
            glm::clamp(flattenAmount, 0.0f, 1.0f)
        );


        // ====================================
        // World mountain boundaries
        // ====================================

        // Our 5-column world runs from -512 to +768.
        // Therefore, the centre is X = 128.

        //constexpr float worldCentreX = 128.0f;

        // Distance from the centre of the world
        //float distanceFromCentre = std::abs(x - worldCentreX);

        // Create a smooth transition towards the edges
        float boundaryBlend = glm::smoothstep(
            settings.BoundaryStart,
            settings.BoundaryEnd,
            distanceFromCentre
        );


        // ====================================
        // Add mountain variation
        // ====================================

        // Use world coordinates so boundaries remain
        // continuous across terrain chunks.

        float boundaryNoise = glm::perlin(
            glm::vec2(
                x * 0.004f + maskOffsetX,
                z * 0.004f + maskOffsetZ
            )
        );

        // Convert noise to approximately 0..1
        boundaryNoise =
            boundaryNoise * 0.5f + 0.5f;

        // Vary the mountain height
        float heightVariation =
            glm::mix(
                0.7f,
                1.3f,
                glm::clamp(boundaryNoise, 0.0f, 1.0f)
            );

        // Calculate boundary mountain height
        float boundaryHeight =
            boundaryBlend *
            settings.BoundaryMountainHeight *
            heightVariation;


        // ====================================
        // Final terrain height
        // ====================================

        return terrainHeight + boundaryHeight;

        /*float mountainHeight =
            mountains *
            settings.HillHeight *
            settings.MountainHeight;

        return hills + mountainHeight * mountainMask;*/
    }



    
}



Terrain::~Terrain()
{
    Destroy();
}

bool Terrain::Generate(int size, float spacing, const glm::vec2& chunkOrigin, const TerrainSettings& settings)
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
                settings
            );
            
            

            // Sample neighbouring heights
            
            float hL = GetTerrainHeight(
                xPos - spacing,
                zPos,
                settings
            );

            float hR = GetTerrainHeight(
                xPos + spacing,
                zPos,
                settings
            );

            float hD = GetTerrainHeight(
                xPos,
                zPos - spacing,
                settings
            );

            float hU = GetTerrainHeight(
                xPos,
                zPos + spacing,
                settings
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

bool Terrain::SaveHeightmap(const std::string& filename, int chunkX, int chunkZ) const
{
    namespace fs = std::filesystem;

    if (m_vertices.empty() || m_gridSize <= 0)
        return false;

    const uint32_t resolution =
        static_cast<uint32_t>(m_gridSize + 1);

    const size_t expectedCount =
        static_cast<size_t>(resolution) * resolution;

    if (m_vertices.size() != expectedCount)
        return false;

    // Create output directory
    fs::path path(filename);

    std::error_code error;

    if (!path.parent_path().empty())
    {
        fs::create_directories(
            path.parent_path(),
            error
        );

        if (error)
        {
            std::cout << "Failed to create heightmap directory\n";
            return false;
        }
    }

    // Open binary file
    std::ofstream file(
        filename,
        std::ios::binary | std::ios::trunc
    );

    if (!file)
    {
        std::cout << "Failed to open heightmap for writing\n";
        return false;
    }

    // ====================================
    // File header
    // ====================================

    const char magic[4] = { 'W', 'H', 'G', 'T' };

    file.write(magic, 4);

    // Format version
    WriteU32(file, 1);

    // Chunk coordinates (preserve signed bits)
    uint32_t xBits;
    uint32_t zBits;

    int32_t signedX = static_cast<int32_t>(chunkX);
    int32_t signedZ = static_cast<int32_t>(chunkZ);

    std::memcpy(&xBits, &signedX, 4);
    std::memcpy(&zBits, &signedZ, 4);

    WriteU32(file, xBits);
    WriteU32(file, zBits);

    // Resolution and grid spacing
    WriteU32(file, resolution);
    WriteF32(file, m_gridSpacing);

    // ====================================
    // Height data
    // ====================================

    for (const auto& vertex : m_vertices)
    {
        WriteF32(file, vertex.Position.y);
    }

    file.flush();

    if (!file.good())
    {
        std::cout << "Failed to write heightmap data\n";
        return false;
    }

    std::cout << "Saved .wheight: "
        << filename << '\n';

    std::cout << "Height samples: "
        << m_vertices.size() << '\n';

    return true;
}


bool Terrain::LoadHeightmap(const std::string& filename, int expectedChunkX, int expectedChunkZ)
{
    if (m_vertices.empty() ||
        m_gridSize <= 0 ||
        m_VBO == 0)
    {
        return false;
    }

    std::ifstream file(
        filename,
        std::ios::binary
    );

    if (!file)
    {
        std::cout << "Failed to open .wheight: "
            << filename << '\n';

        return false;
    }

    // ====================================
    // Read file header
    // ====================================

    char magic[4];

    if (!file.read(magic, 4))
        return false;

    if (std::memcmp(magic, "WHGT", 4) != 0)
    {
        std::cout << "Invalid heightmap signature\n";
        return false;
    }

    uint32_t version = 0;
    uint32_t xBits = 0;
    uint32_t zBits = 0;
    uint32_t resolution = 0;

    float spacing = 0.0f;

    if (!ReadU32(file, version) ||
        !ReadU32(file, xBits) ||
        !ReadU32(file, zBits) ||
        !ReadU32(file, resolution) ||
        !ReadF32(file, spacing))
    {
        std::cout << "Invalid heightmap header\n";
        return false;
    }

    int32_t chunkX;
    int32_t chunkZ;

    std::memcpy(&chunkX, &xBits, 4);
    std::memcpy(&chunkZ, &zBits, 4);

    // ====================================
    // Validate file
    // ====================================

    const uint32_t expectedResolution =
        static_cast<uint32_t>(m_gridSize + 1);

    if (version != 1 ||
        chunkX != expectedChunkX ||
        chunkZ != expectedChunkZ ||
        resolution != expectedResolution ||
        !std::isfinite(spacing) ||
        spacing != m_gridSpacing)
    {
        std::cout << "Heightmap header mismatch\n";
        return false;
    }

    const size_t heightCount =
        static_cast<size_t>(resolution) * resolution;

    if (heightCount != m_vertices.size())
        return false;

    // ====================================
    // Read heights into temporary storage
    // ====================================

    std::vector<float> heights(heightCount);

    for (size_t i = 0; i < heightCount; ++i)
    {
        if (!ReadF32(file, heights[i]))
        {
            std::cout << "Incomplete heightmap data\n";
            return false;
        }

        if (!std::isfinite(heights[i]))
        {
            std::cout << "Invalid terrain height\n";
            return false;
        }
    }

    // Reject unexpected trailing data
    if (file.peek() != EOF)
    {
        std::cout << "Unexpected heightmap data\n";
        return false;
    }

    // ====================================
    // Update a copy of terrain vertices
    // ====================================

    std::vector<TerrainVertex> updated = m_vertices;

    for (size_t i = 0; i < heightCount; ++i)
    {
        updated[i].Position.y = heights[i];
    }

    // ====================================
    // Recalculate normals
    // ====================================

    const int rowSize = m_gridSize + 1;

    auto GetHeight = [&](int x, int z)
    {
        x = std::clamp(x, 0, m_gridSize);
        z = std::clamp(z, 0, m_gridSize);

        return updated[
            z * rowSize + x
        ].Position.y;
    };

    for (int z = 0; z <= m_gridSize; ++z)
    {
        for (int x = 0; x <= m_gridSize; ++x)
        {
            float hL = GetHeight(x - 1, z);
            float hR = GetHeight(x + 1, z);

            float hD = GetHeight(x, z - 1);
            float hU = GetHeight(x, z + 1);

            float dx =
                (x == 0 || x == m_gridSize)
                ? m_gridSpacing
                : 2.0f * m_gridSpacing;

            float dz =
                (z == 0 || z == m_gridSize)
                ? m_gridSpacing
                : 2.0f * m_gridSpacing;

            glm::vec3 normal(
                (hL - hR) / dx,
                1.0f,
                (hD - hU) / dz
            );

            updated[
                z * rowSize + x
            ].Normal = glm::normalize(normal);
        }
    }

    // ====================================
    // Upload to OpenGL
    // ====================================

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        updated.size() * sizeof(TerrainVertex),
        updated.data()
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_vertices = std::move(updated);

    std::cout << "Loaded .wheight: "
        << filename << '\n';

    return true;
}


bool Terrain::SaveHeightmapPNG(const std::string& filename) const
{
    if (m_vertices.empty() || m_gridSize <= 0)
        return false;

    const int imageSize = m_gridSize + 1;

    const size_t expectedVertices =
        static_cast<size_t>(imageSize) * imageSize;

    if (m_vertices.size() != expectedVertices)
        return false;

    // ====================================
    // Find minimum and maximum heights
    // ====================================

    float minHeight = m_vertices[0].Position.y;
    float maxHeight = minHeight;

    for (const auto& vertex : m_vertices)
    {
        float height = vertex.Position.y;

        minHeight = std::min(minHeight, height);
        maxHeight = std::max(maxHeight, height);
    }

    // ====================================
    // Create greyscale image
    // ====================================

    std::vector<unsigned char> pixels(
        expectedVertices
    );

    float heightRange = maxHeight - minHeight;

    for (size_t i = 0; i < m_vertices.size(); ++i)
    {
        float height = m_vertices[i].Position.y;

        float normalised = 0.0f;

        if (heightRange > 0.000001f)
        {
            normalised =
                (height - minHeight) / heightRange;
        }

        normalised = std::clamp(
            normalised,
            0.0f,
            1.0f
        );

        pixels[i] = static_cast<unsigned char>(
            std::lround(normalised * 255.0f)
            );
    }

    // ====================================
    // Create output directory
    // ====================================

    namespace fs = std::filesystem;

    fs::path outputPath(filename);

    std::error_code error;

    if (!outputPath.parent_path().empty())
    {
        fs::create_directories(
            outputPath.parent_path(),
            error
        );

        if (error)
        {
            std::cout << "Failed to create heightmap folder: "
                << error.message() << std::endl;

            return false;
        }
    }

    // ====================================
    // Save PNG
    // ====================================

    int result = stbi_write_png(
        filename.c_str(),
        imageSize,
        imageSize,
        1,
        pixels.data(),
        imageSize
    );

    if (result == 0)
    {
        std::cout << "Failed to save heightmap: "
            << filename << std::endl;

        return false;
    }

    // ====================================
    // Save height range
    // ====================================

    // The PNG stores values from 0 to 255.
    // We need the original height range to
    // reconstruct elevations later.

    fs::path infoPath = outputPath;
    infoPath.replace_extension(".txt");

    std::ofstream info(infoPath);

    if (!info)
    {
        std::cout << "Failed to save height range"
            << std::endl;

        return false;
    }

    info << "version=1\n";
    info << "minHeight=" << minHeight << "\n";
    info << "maxHeight=" << maxHeight << "\n";
    info << "resolution=" << imageSize << "\n";

    if (!info.good())
        return false;

    std::cout << "Heightmap saved: "
        << filename << std::endl;

    std::cout << "Height range: "
        << minHeight << " to "
        << maxHeight << std::endl;

    return true;
}


bool Terrain::LoadHeightmapPNG(
    const std::string& filename
)
{
    namespace fs = std::filesystem;

    if (m_vertices.empty() ||
        m_VBO == 0 ||
        m_gridSize <= 0)
    {
        std::cout << "Terrain is not initialized\n";
        return false;
    }

    // =====================================
    // 1. Read heightmap metadata
    // =====================================

    fs::path infoPath(filename);
    infoPath.replace_extension(".txt");

    std::ifstream info(infoPath);

    if (!info)
    {
        std::cout << "Failed to open heightmap metadata: "
            << infoPath.string() << '\n';
        return false;
    }

    float minHeight = 0.0f;
    float maxHeight = 0.0f;
    int resolution = 0;
    int version = 0;

    bool hasMin = false;
    bool hasMax = false;
    bool hasResolution = false;

    std::string line;

    try
    {
        while (std::getline(info, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            size_t separator = line.find('=');

            if (separator == std::string::npos)
                continue;

            std::string key = line.substr(0, separator);
            std::string value = line.substr(separator + 1);

            if (key == "version")
                version = std::stoi(value);

            else if (key == "minHeight")
            {
                minHeight = std::stof(value);
                hasMin = true;
            }
            else if (key == "maxHeight")
            {
                maxHeight = std::stof(value);
                hasMax = true;
            }
            else if (key == "resolution")
            {
                resolution = std::stoi(value);
                hasResolution = true;
            }
        }
    }
    catch (const std::exception&)
    {
        std::cout << "Invalid heightmap metadata\n";
        return false;
    }

    const int expectedSize = m_gridSize + 1;

    if (version != 1 ||
        !hasMin || !hasMax || !hasResolution ||
        minHeight > maxHeight ||
        resolution != expectedSize)
    {
        std::cout << "Heightmap metadata mismatch\n";
        return false;
    }

    // =====================================
    // 2. Load greyscale PNG
    // =====================================

    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(false);

    unsigned char* pixels = stbi_load(
        filename.c_str(),
        &width,
        &height,
        &channels,
        1
    );

    if (!pixels)
    {
        std::cout << "Failed to load heightmap: "
            << filename << '\n';
        return false;
    }

    if (width != expectedSize ||
        height != expectedSize ||
        channels != 1)
    {
        std::cout << "Heightmap image mismatch\n";
        stbi_image_free(pixels);
        return false;
    }

    // =====================================
    // 3. Reconstruct vertex heights
    // =====================================

    std::vector<TerrainVertex> newVertices = m_vertices;

    const float heightRange =
        maxHeight - minHeight;

    for (size_t i = 0; i < newVertices.size(); ++i)
    {
        float grey =
            static_cast<float>(pixels[i]) / 255.0f;

        float worldHeight =
            minHeight + grey * heightRange;

        newVertices[i].Position.y = worldHeight;
    }

    stbi_image_free(pixels);

    // =====================================
    // 4. Recalculate surface normals
    // =====================================

    const int rowSize = expectedSize;

    auto GetHeight = [&](int x, int z) -> float
    {
        x = std::clamp(x, 0, m_gridSize);
        z = std::clamp(z, 0, m_gridSize);

        return newVertices[
            z * rowSize + x
        ].Position.y;
    };

    for (int z = 0; z <= m_gridSize; ++z)
    {
        for (int x = 0; x <= m_gridSize; ++x)
        {
            float hL = GetHeight(x - 1, z);
            float hR = GetHeight(x + 1, z);
            float hD = GetHeight(x, z - 1);
            float hU = GetHeight(x, z + 1);

            // Use one-sided distance at outer edges
            float dx = (x == 0 || x == m_gridSize)
                ? m_gridSpacing
                : 2.0f * m_gridSpacing;

            float dz = (z == 0 || z == m_gridSize)
                ? m_gridSpacing
                : 2.0f * m_gridSpacing;

            glm::vec3 normal(
                (hL - hR) / dx,
                1.0f,
                (hD - hU) / dz
            );

            newVertices[
                z * rowSize + x
            ].Normal = glm::normalize(normal);
        }
    }

    // =====================================
    // 5. Update OpenGL vertex buffer
    // =====================================

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        newVertices.size() * sizeof(TerrainVertex),
        newVertices.data()
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Update our CPU-side vertices as well.
    // The walking camera queries these heights.
    m_vertices = std::move(newVertices);

    std::cout << "Heightmap loaded: "
        << filename << '\n';

    std::cout << "Height range: "
        << minHeight << " to "
        << maxHeight << '\n';

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