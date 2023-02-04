#include "Mesh.h"
#include "GL/glew.h"
#include "OBJ_Loader.h"
#include "glm/common.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"


void ObjMesh::Load_OBJ(const char* filename) {
	objl::Loader loader;
	bool success = loader.LoadFile(filename);
	if (success) {
		int vert_count = loader.LoadedVertices.size();
		m_count = loader.LoadedIndices.size();

		glCreateVertexArrays(1, &m_vao);

		glCreateBuffers(1, &m_vertex_buffer);
		glNamedBufferStorage(m_vertex_buffer, vert_count * sizeof(objl::Vertex), &loader.LoadedVertices[0], 0);

		glCreateBuffers(1, &m_index_buffer);
		glNamedBufferStorage(m_index_buffer, loader.LoadedIndices.size() * sizeof(GLuint), &loader.LoadedIndices[0], 0);

		glVertexArrayAttribBinding(m_vao, 0, 0);
		glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(objl::Vertex, Position));
		glEnableVertexArrayAttrib(m_vao, 0);

		glVertexArrayAttribBinding(m_vao, 1, 0);
		glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(objl::Vertex, Normal));
		glEnableVertexArrayAttrib(m_vao, 1);

		glVertexArrayAttribBinding(m_vao, 2, 0);
		glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(objl::Vertex, TextureCoordinate));
		glEnableVertexArrayAttrib(m_vao, 2);

		glVertexArrayVertexBuffer(m_vao, 0, m_vertex_buffer, 0, sizeof(objl::Vertex));
		glVertexArrayElementBuffer(m_vao, m_index_buffer);

		glBindVertexArray(0);
	}
	else {
		std::cout << "Failed to load mesh: " << filename << std::endl;
	}
}

void ObjMesh::OnUpdate(glm::mat4 mvp, f64 time, WindowXY resolution) {
	m_mvp = mvp;
	m_time = time;
	m_resolution = resolution;
}

void ObjMesh::OnDraw(GLuint shader) {
	glUseProgram(shader);
	glBindVertexArray(m_vao);
	glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(m_mvp));
	glUniform1f(5, (float)m_time);
	glUniform2i(6, m_resolution.width, m_resolution.height);
	glDrawElements(GL_TRIANGLES, m_count, GL_UNSIGNED_INT, (void*)0);
}