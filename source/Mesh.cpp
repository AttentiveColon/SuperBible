#include "Mesh.h"
#include "GL/glew.h"
#include "OBJ_Loader.h"
#include "glm/common.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"


namespace SB {
	struct Vertex {
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TextureCoordinate;
		glm::vec3 Tangent;
	};
}

void ObjMesh::Load_OBJ(const char* filename) {
	objl::Loader loader;
	bool success = loader.LoadFile(filename);
	if (success) {
		int vert_count = loader.LoadedVertices.size();
		m_count = loader.LoadedIndices.size();

		std::vector<SB::Vertex> vertices;

		for (size_t i = 0; i < m_count; i += 3) {
			objl::Vertex& v0 = loader.LoadedVertices[loader.LoadedIndices[i]];
			objl::Vertex& v1 = loader.LoadedVertices[loader.LoadedIndices[i + 1]];
			objl::Vertex& v2 = loader.LoadedVertices[loader.LoadedIndices[i + 2]];

			glm::vec3 edge1 = glm::vec3(v1.Position.X, v1.Position.Y, v1.Position.Z) - glm::vec3(v0.Position.X, v0.Position.Y, v0.Position.Z);
			glm::vec3 edge2 = glm::vec3(v2.Position.X, v2.Position.Y, v2.Position.Z) - glm::vec3(v0.Position.X, v0.Position.Y, v0.Position.Z);
			glm::vec2 deltaUV1 = glm::vec2(v1.TextureCoordinate.X, v1.TextureCoordinate.Y) - glm::vec2(v0.TextureCoordinate.X, v0.TextureCoordinate.Y);
			glm::vec2 deltaUV2 = glm::vec2(v2.TextureCoordinate.X, v2.TextureCoordinate.Y) - glm::vec2(v0.TextureCoordinate.X, v0.TextureCoordinate.Y);

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent = glm::normalize(tangent);

			SB::Vertex v00 = { glm::vec3(v0.Position.X, v0.Position.Y, v0.Position.Z), glm::vec3(v0.Normal.X, v0.Normal.Y, v0.Normal.Z), glm::vec2(v0.TextureCoordinate.X, v0.TextureCoordinate.Y), tangent };
			SB::Vertex v01 = { glm::vec3(v1.Position.X, v1.Position.Y, v1.Position.Z), glm::vec3(v1.Normal.X, v1.Normal.Y, v1.Normal.Z), glm::vec2(v1.TextureCoordinate.X, v1.TextureCoordinate.Y), tangent };
			SB::Vertex v02 = { glm::vec3(v2.Position.X, v2.Position.Y, v2.Position.Z), glm::vec3(v2.Normal.X, v2.Normal.Y, v2.Normal.Z), glm::vec2(v2.TextureCoordinate.X, v2.TextureCoordinate.Y), tangent };

			/*v0.Tangent += tangent;
			v1.Tangent += tangent;
			v2.Tangent += tangent;*/

			vertices.push_back(v00);
			vertices.push_back(v01);
			vertices.push_back(v02);
		}

		glCreateVertexArrays(1, &m_vao);

		glCreateBuffers(1, &m_vertex_buffer);
		glNamedBufferStorage(m_vertex_buffer, vert_count * sizeof(SB::Vertex), &vertices[0], 0);

		glCreateBuffers(1, &m_index_buffer);
		glNamedBufferStorage(m_index_buffer, loader.LoadedIndices.size() * sizeof(GLuint), &loader.LoadedIndices[0], 0);

		glVertexArrayAttribBinding(m_vao, 0, 0);
		glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(SB::Vertex, Position));
		glEnableVertexArrayAttrib(m_vao, 0);

		glVertexArrayAttribBinding(m_vao, 1, 0);
		glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(SB::Vertex, Normal));
		glEnableVertexArrayAttrib(m_vao, 1);

		glVertexArrayAttribBinding(m_vao, 2, 0);
		glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(SB::Vertex, TextureCoordinate));
		glEnableVertexArrayAttrib(m_vao, 2);

		glVertexArrayAttribBinding(m_vao, 3, 0);
		glVertexArrayAttribFormat(m_vao, 3, 3, GL_FLOAT, GL_FALSE, offsetof(SB::Vertex, Tangent));
		glEnableVertexArrayAttrib(m_vao, 3);

		glVertexArrayVertexBuffer(m_vao, 0, m_vertex_buffer, 0, sizeof(SB::Vertex));
		glVertexArrayElementBuffer(m_vao, m_index_buffer);

		glBindVertexArray(0);
	}
	else {
		std::cout << "Failed to load mesh: " << filename << std::endl;
	}
}

void ObjMesh::OnUpdate(f64 dt) {

}

void ObjMesh::OnDraw() {
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, m_count, GL_UNSIGNED_INT, (void*)0);
}

void ObjMesh::OnDraw(int instances) {
	glBindVertexArray(m_vao);
	glDrawElementsInstanced(GL_TRIANGLES, m_count, GL_UNSIGNED_INT, (void*)0, instances);
}