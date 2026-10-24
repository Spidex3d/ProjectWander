#include "graphics/Material.h"

#include <stb/stb_image.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

Material::~Material()
{
    Destroy();
}

bool Material::Load(const std::string& matFile)
{
    Destroy();

    std::ifstream file(matFile);

    if (!file.is_open())
    {
        std::cout << "Failed to open material: "
            << matFile << std::endl;

        return false;
    }

    std::unordered_map<std::string, std::string> properties;

    std::string line;

    // Read material properties
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty() || line[0] == '#')
            continue;

        size_t separator = line.find('=');

        if (separator == std::string::npos)
            continue;

        std::string key = line.substr(0, separator);
        std::string value = line.substr(separator + 1);

        properties[key] = value;
    }

    m_name = properties["name"];

    // Get material directory
    fs::path materialPath(matFile);
    fs::path materialDirectory = materialPath.parent_path();

    // Find albedo texture
    auto it = properties.find("albedo");

    if (it == properties.end() || it->second.empty())
    {
        std::cout << "Material has no albedo texture"
            << std::endl;

        return false;
    }

    fs::path albedoPath =
        materialDirectory / it->second;

    //m_albedo = LoadTexture(albedoPath.string());
    m_albedo = LoadTexture(albedoPath.string(), true);

    if (m_albedo == 0)
        return false;

    std::cout << "Material loaded: " << m_name << std::endl;

    // =======================================
    // Load normal map
    // =======================================

    auto normalIt = properties.find("normal");

    if (normalIt != properties.end() &&
        !normalIt->second.empty())
    {
        fs::path normalPath =
            materialDirectory / normalIt->second;

        m_normal = LoadTexture(
            normalPath.string(),
            false
        );

        if (m_normal == 0)
        {
            std::cout << "Failed to load normal map: "
                << normalPath.string()
                << std::endl;

            Destroy();
            return false;
        }
    }

    return true;
}

void Material::BindNormal(GLuint textureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);

    glBindTexture(
        GL_TEXTURE_2D,
        m_normal
    );
}

GLuint Material::LoadTexture(const std::string& path, bool isColorTexture)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    // Keep TextureBox image orientation unchanged for now.
    stbi_set_flip_vertically_on_load(false);

    unsigned char* data = stbi_load(
        path.c_str(),
        &width,
        &height,
        &channels,
        0
    );

    if (!data)
    {
        std::cout << "Failed to load texture: "
            << path << std::endl;

        return 0;
    }

    if (channels != 3 && channels != 4)
    {
        std::cout << "Unsupported texture channels: "
            << channels << std::endl;

        stbi_image_free(data);
        return 0;
    }

    GLenum format = channels == 4 ? GL_RGBA : GL_RGB;

    GLuint texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Texture wrapping
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    // Texture filtering
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    // Correct row alignment for RGB images
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // ############################################
    // Determine image format
    //GLenum format = channels == 4 ? GL_RGBA : GL_RGB;

    // Albedo uses sRGB.
    // Normal maps use linear RGB.
    GLenum internalFormat;

    if (isColorTexture)
    {
        internalFormat =
            channels == 4 ? GL_SRGB8_ALPHA8 : GL_SRGB8;
    }
    else
    {
        internalFormat =
            channels == 4 ? GL_RGBA8 : GL_RGB8;
    }

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );


    /*glTexImage2D(
        GL_TEXTURE_2D,
        0,
        channels == 4 ? GL_SRGB8_ALPHA8 : GL_SRGB8,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );*/

    glGenerateMipmap(GL_TEXTURE_2D);

    // Restore default alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    std::cout << "Loaded texture: "
        << path << std::endl;

    return texture;
}

void Material::Bind(GLuint textureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_albedo);
}

void Material::Unbind(GLuint textureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Material::Destroy()
{
    // Delete albedo texture
    if (m_albedo != 0)
    {
        glDeleteTextures(1, &m_albedo);
        m_albedo = 0;
    }

    // Delete normal texture
    if (m_normal != 0)
    {
        glDeleteTextures(1, &m_normal);
        m_normal = 0;
    }

    m_name.clear();
}