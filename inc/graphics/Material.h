#pragma once

#include <glad/glad.h>
#include <string>

class Material
{
public:
    Material() = default;
    ~Material();

    // Prevent copying OpenGL texture handles
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    bool Load(const std::string& matFile);
    void BindNormal(GLuint textureUnit) const;

    void Bind(GLuint textureUnit = 0) const;
    void Unbind(GLuint textureUnit = 0) const;

    void Destroy();

    const std::string& GetName() const
    {
        return m_name;
    }

    bool IsValid() const
    {
        return m_albedo != 0;
    }

private:
    GLuint LoadTexture(const std::string& path, bool isColorTexture);

    std::string m_name;

    GLuint m_albedo = 0;
    GLuint m_normal = 0;
};