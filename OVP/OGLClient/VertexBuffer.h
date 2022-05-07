#pragma once
//#include "glad.h"

class VertexBuffer {
public:
    VertexBuffer(void *data, uint32_t size);
    ~VertexBuffer();

    void Bind() const;
    void UnBind() const;

    void *Map() const;
    void UnMap() const;

    void Update(void *data, int size);
private:
    GLuint VBID;
};

class IndexBuffer {
    using index_type = uint16_t;
public:
    IndexBuffer(index_type* indices, uint32_t count);
    ~IndexBuffer();

    void Bind() const;
    void UnBind() const;
    void *Map() const;
    void UnMap() const;
    index_type GetCount() const { return count; }
private:
    GLuint IBID;
    index_type count;
};

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    void Bind() const;
    void UnBind() const;
    IndexBuffer *GetIBO() const { return IBO; }
private:
    GLuint VAID;
    IndexBuffer *IBO;
    VertexBuffer *VBO;
};
