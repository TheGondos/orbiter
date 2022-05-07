#include "glad.h"
#include "VertexBuffer.h"
#include <stdio.h>
#include <stdlib.h>
static void CheckError(const char *s) {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	// Process/log the error.
		printf("GLError: %s - 0x%04X\n", s, err);
        abort();
        exit(-1);
	}
}


VertexBuffer::VertexBuffer(void *data, uint32_t size)
{
    glGenBuffers(1, &VBID);
    CheckError("VertexBuffer::glGenBuffers");
    glBindBuffer(GL_ARRAY_BUFFER, VBID);
    CheckError("VertexBuffer::glBindBuffer");
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
    CheckError("VertexBuffer::glBufferData");
}

void VertexBuffer::Update(void *data, int size) {
    glBufferSubData( GL_ARRAY_BUFFER , 0 , size, data );
    CheckError("VertexBuffer::glBufferSubData");
}

VertexBuffer::~VertexBuffer() 
{
    glDeleteBuffers(1, &VBID);
    CheckError("~VertexBuffer::glDeleteBuffers");
}
void VertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, VBID);
    CheckError("VertexBuffer::Bind : glBindBuffer");
}
void VertexBuffer::UnBind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CheckError("VertexBuffer::UnBind : glBindBuffer");
}

void *VertexBuffer::Map() const
{
    Bind();
    void *map = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
    CheckError("VertexBuffer::Map : glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE)");
    return map;
}
void VertexBuffer::UnMap() const
{
    glUnmapBuffer(GL_ARRAY_BUFFER);
    CheckError("VertexBuffer::UnMap : glUnmapBuffer(GL_ARRAY_BUFFER)");
    UnBind();
}

IndexBuffer::IndexBuffer(index_type* indices, uint32_t c)
{
    count = c;
    glGenBuffers(1, &IBID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(index_type), indices, GL_STATIC_DRAW);
    CheckError("IndexBuffer::IndexBuffer()");
}
IndexBuffer::~IndexBuffer() 
{
    glDeleteBuffers(1, &IBID);
}
void IndexBuffer::Bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBID);
    CheckError("IndexBuffer::Bind : glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBID)");
}
void IndexBuffer::UnBind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CheckError("IndexBuffer::UnBind : glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0)");
}
void *IndexBuffer::Map() const
{
    Bind();
    void *map = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
    CheckError("IndexBuffer::Map : glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE)");
    return map;
}
void IndexBuffer::UnMap() const
{
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    CheckError("IndexBuffer::UnMap : glUnmapBuffer(GL_ARRAY_BUFFER)");
    UnBind();
}

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &VAID);
    CheckError("VertexArray::VertexArray : glGenVertexArrays");
}
VertexArray::~VertexArray() 
{
    glDeleteVertexArrays(1, &VAID);
    CheckError("VertexArray::~VertexArray : glDeleteVertexArrays");
}
void VertexArray::Bind() const
{
    glBindVertexArray(VAID);
    CheckError("VertexArray::Bind : glBindVertexArray");
}
void VertexArray::UnBind() const
{
    glBindVertexArray(0);
    CheckError("VertexArray::UnBind : glBindVertexArray(0)");
}
