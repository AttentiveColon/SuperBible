#pragma once
#include "GL_Helpers.h"
#include "glm/common.hpp"
#include "glm/glm.hpp"

struct ObjMesh {
    GLuint m_vao;
    GLuint m_vertex_buffer;
    GLuint m_index_buffer;
    GLsizei m_count;

    f64 m_time;
    WindowXY m_resolution;
    glm::mat4 m_mvp;

    void Load_OBJ(const char* filename);
    void OnUpdate(f64 dt);
    void OnDraw();
    void OnDraw(int instances);
};