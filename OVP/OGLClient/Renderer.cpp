#include "glad.h"
#include "Renderer.h"
#include "Shader.h"
#include "OGLCamera.h"
#include "VertexBuffer.h"

glm::fmat4 Renderer::ViewProjectionMatrix;

void Renderer::Init()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
}

void Renderer::Shutdown()
{
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
    glViewport(0, 0, width, height);
}

void Renderer::BeginScene(const OGLCamera *camera)
{
    ViewProjectionMatrix = *camera->GetViewProjectionMatrix();
}

void Renderer::EndScene()
{
}

void Renderer::Submit(const Shader *shader, const VertexArray *vertexArray, const glm::mat4& transform)
{
    shader->Bind();
    shader->SetMat4("u_ViewProjection", ViewProjectionMatrix);
    shader->SetMat4("u_Transform", transform);

    vertexArray->Bind();
    uint32_t count = vertexArray->GetIBO()->GetCount();
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}
