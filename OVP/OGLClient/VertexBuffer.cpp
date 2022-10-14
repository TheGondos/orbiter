#include "glad.h"
#include "VertexBuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include "Renderer.h"

VertexBuffer::VertexBuffer(void *data, uint32_t size)
{
    glGenBuffers(1, &VBID);
    Renderer::CheckError("VertexBuffer::glGenBuffers");
    glBindBuffer(GL_ARRAY_BUFFER, VBID);
    Renderer::CheckError("VertexBuffer::glBindBuffer");
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
    Renderer::CheckError("VertexBuffer::glBufferData");
}

void VertexBuffer::Update(void *data, int size) {
    glBufferSubData( GL_ARRAY_BUFFER , 0 , size, data );
    Renderer::CheckError("VertexBuffer::glBufferSubData");
}

VertexBuffer::~VertexBuffer() 
{
    glDeleteBuffers(1, &VBID);
    Renderer::CheckError("~VertexBuffer::glDeleteBuffers");
}
void VertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, VBID);
    Renderer::CheckError("VertexBuffer::Bind : glBindBuffer");
}
void VertexBuffer::UnBind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    Renderer::CheckError("VertexBuffer::UnBind : glBindBuffer");
}

void *VertexBuffer::Map() const
{
    Bind();
    void *map = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    Renderer::CheckError("VertexBuffer::Map : glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE)");
    return map;
}
void VertexBuffer::UnMap() const
{
    glUnmapBuffer(GL_ARRAY_BUFFER);
    Renderer::CheckError("VertexBuffer::UnMap : glUnmapBuffer(GL_ARRAY_BUFFER)");
    UnBind();
}

IndexBuffer::IndexBuffer(index_type* indices, uint32_t c)
{
    count = c;
    glGenBuffers(1, &IBID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(index_type), indices, GL_STATIC_DRAW);
    Renderer::CheckError("IndexBuffer::IndexBuffer()");
}
IndexBuffer::~IndexBuffer() 
{
    glDeleteBuffers(1, &IBID);
}
void IndexBuffer::Bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBID);
    Renderer::CheckError("IndexBuffer::Bind : glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBID)");
}
void IndexBuffer::UnBind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    Renderer::CheckError("IndexBuffer::UnBind : glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
}
void *IndexBuffer::Map() const
{
    Bind();
    void *map = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
    Renderer::CheckError("IndexBuffer::Map : glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE)");
    return map;
}
void IndexBuffer::UnMap() const
{
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    Renderer::CheckError("IndexBuffer::UnMap : glUnmapBuffer(GL_ARRAY_BUFFER)");
    UnBind();
}

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &VAID);
    Renderer::CheckError("VertexArray::VertexArray : glGenVertexArrays");
}
VertexArray::~VertexArray() 
{
    glDeleteVertexArrays(1, &VAID);
    Renderer::CheckError("VertexArray::~VertexArray : glDeleteVertexArrays");
}
void VertexArray::Bind() const
{
    glBindVertexArray(VAID);
    Renderer::CheckError("VertexArray::Bind : glBindVertexArray");
}
void VertexArray::UnBind() const
{
    glBindVertexArray(0);
    Renderer::CheckError("VertexArray::UnBind : glBindVertexArray(0)");
}
