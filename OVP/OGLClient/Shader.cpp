#include "glad.h"
#include "Shader.h"
#include "Shadinclude.hpp"
#include <vector>
#include <sstream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

#define OGLLOG(...) fprintf (stderr, __VA_ARGS__)

Shader::Shader(const std::string& vertexFile, const std::string& fragmentFile)
{
    /*
    std::string vf = std::string("Modules/OGLClient/") + vertexFile;
    std::ifstream isv(vf);
    std::stringstream bufv;
    bufv << isv.rdbuf();

    std::string ff = std::string("Modules/OGLClient/") + fragmentFile;
    std::ifstream isf(ff);
    std::stringstream buff;
    buff << isf.rdbuf();
*/
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
 
    std::string tmp = Shadinclude::load(std::string("Modules/OGLClient/") + vertexFile);

//    std::string tmp = bufv.str();
    const GLchar* source = tmp.c_str();

    glShaderSource(vertexShader, 1, &source, 0);
    glCompileShader(vertexShader);

    GLint isCompiled = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]);

        glDeleteShader(vertexShader);

        printf("Vertex shader compilation failure! %s\n", &infoLog[0]);
        abort();
        exit(-1);
        return;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    //tmp = buff.str();
    tmp = Shadinclude::load(std::string("Modules/OGLClient/") + fragmentFile);

    source = tmp.c_str();
    glShaderSource(fragmentShader, 1, &source, 0);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]);

        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);

        printf("Fragment shader compilation failure! %s\n", &infoLog[0]);
        abort();
        exit(-1);
        return;
    }

    ShaderID = glCreateProgram();

    glAttachShader(ShaderID, vertexShader);
    glAttachShader(ShaderID, fragmentShader);

    glLinkProgram(ShaderID);

    GLint isLinked = 0;
    glGetProgramiv(ShaderID, GL_LINK_STATUS, (int*)&isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(ShaderID, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(ShaderID, maxLength, &maxLength, &infoLog[0]);

        glDeleteProgram(ShaderID);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        printf("Shader link failure! %s\n", &infoLog[0]);
        abort();
        exit(-1);
        return;
    }

    glDetachShader(ShaderID, vertexShader);
    glDetachShader(ShaderID, fragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(ShaderID);
}
/*
void Shader::Bind() const
{
    glUseProgram(ShaderID);
    GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glUseProgram - 0x%04X\n", err);
        abort();
        exit(-1);
	}
}
void Shader::UnBind() const
{
    glUseProgram(0);
    GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glUseProgram(0) - 0x%04X\n", err);
        abort();
        exit(-1);
	}
}
*/

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const
{
    GLint location = glGetUniformLocation(ShaderID, name.c_str());
    if(location == -1) {
        printf("glGetUniformLocation failed for %s\n", name.c_str());
        //abort();
        //exit(-1);
        return;
    }
    GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glGetUniformLocation - 0x%04X\n", err);
        abort();
        exit(-1);
	}

    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glUniformMatrix4fv - 0x%04X loc=%d %s\n", err, location, name.c_str());
        abort();
        exit(-1);
	}
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    GLint location = glGetUniformLocation(ShaderID, name.c_str());
    if(location == -1) {
        printf("glGetUniformLocation failed for %s\n", name.c_str());
        return;
        exit(-1);
    }
    GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glGetUniformLocation - 0x%04X\n", err);
        abort();
        exit(-1);
	}

    glUniform3fv(location, 1, glm::value_ptr(value));
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glUniformMatrix4fv - 0x%04X loc=%d %s\n", err, location, name.c_str());
        abort();
        exit(-1);
	}
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const
{
    GLint location = glGetUniformLocation(ShaderID, name.c_str());
    if(location == -1) {
        printf("glGetUniformLocation failed for %s\n", name.c_str());
        return;
        exit(-1);
    }
    GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glGetUniformLocation - 0x%04X\n", err);
        abort();
        exit(-1);
	}

    glUniform4fv(location, 1, glm::value_ptr(value));
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glUniformMatrix4fv - 0x%04X loc=%d %s\n", err, location, name.c_str());
        abort();
        exit(-1);
	}
}
void Shader::SetFloat(const std::string& name, float value) const
{
    GLint location = glGetUniformLocation(ShaderID, name.c_str());
    if(location == -1) {
        printf("glGetUniformLocation failed for %s\n", name.c_str());
        return;
        exit(-1);
    }
    GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glGetUniformLocation - 0x%04X\n", err);
        abort();
        exit(-1);
	}

    glUniform1fv(location, 1, &value);
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: glUniform1fv - 0x%04X loc=%d %s\n", err, location, name.c_str());
        abort();
        exit(-1);
	}
}
