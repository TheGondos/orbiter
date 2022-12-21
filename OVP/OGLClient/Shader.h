#pragma once

#include <glm/glm.hpp>
#include <string>
#include <GLFW/glfw3.h>

class  Shader
{
    using location = GLint;
public:
    Shader(const std::string& vertexFile, const std::string& fragmentFile);
    ~Shader();

//    void Bind() const;
//    void UnBind() const;

    void SetMat4(const std::string& name, const glm::mat4& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetInt(const std::string& name, int value);
/*
    virtual void SetIntArray(const std::string& name, int* values, uint32_t count);
    virtual void SetFloat(const std::string& name, float value);
    virtual void SetFloat2(const std::string& name, const glm::vec2& value);
    virtual void SetFloat4(const std::string& name, const glm::vec4& value);

    void UploadUniformInt(const std::string& name, int value);
    void UploadUniformIntArray(const std::string& name, int* values, uint32_t count);

    void UploadUniformFloat(const std::string& name, float value);
    void UploadUniformFloat2(const std::string& name, const glm::vec2& value);
    void UploadUniformFloat3(const std::string& name, const glm::vec3& value);
    void UploadUniformFloat4(const std::string& name, const glm::vec4& value);

    void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
    void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);
*/
    GLuint ShaderID;
};
