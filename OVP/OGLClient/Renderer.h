#pragma once
#include <glm/glm.hpp>

class OGLCamera;
class Shader;
class VertexArray;

class Renderer
{
public:
    static void Init();
    static void Shutdown();
    
    static void OnWindowResize(uint32_t width, uint32_t height);

    static void BeginScene(const OGLCamera *camera);
    static void EndScene();

    static void Submit(const Shader *shader, const VertexArray *vertexArray, const glm::mat4& transform = glm::mat4(1.0f));
private:
    static glm::fmat4 ViewProjectionMatrix;
};
