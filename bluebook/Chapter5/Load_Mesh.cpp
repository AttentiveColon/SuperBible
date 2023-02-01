#include "Defines.h"
#ifdef LOAD_MESH
#include "System.h"
#include "OBJ_Loader.h"
#include "glm/common.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"



static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

uniform mat4 mvp;

out vec3 vs_normal;
out vec2 vs_uv;

void main() 
{
	vec4 new_pos = mvp * vec4(position, 1.0);
	gl_Position = new_pos;
	vs_normal = normal;
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in vec3 vs_normal;
in vec2 vs_uv;

out vec4 color;

void main() 
{
	color = vec4(0.5 * vs_normal.y, 0.5 * vs_normal.x, 0.5 * vs_normal.z, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Mesh {
	GLuint m_vao;
	std::vector<GLuint> m_vertex_buffers;
	GLuint m_vertex_buffer;
	GLuint m_index_buffer;
	GLsizei m_count;
	GLuint m_program;

	glm::mat4 m_mvp;

	void OnInit2(const char* filename) {
		m_program = LoadShaders(shader_text);
		objl::Loader loader;
		bool success = loader.LoadFile(filename);
		if (success) {
			GLsizei mesh_count = loader.LoadedMeshes.size();
			for (int i = 0; i < mesh_count; ++i) {
				m_vertex_buffers.push_back(0);
			}

			glCreateVertexArrays(1, &m_vao);
			glCreateBuffers(1, &m_vertex_buffer);
			glCreateBuffers(1, &m_index_buffer);

			m_count = loader.LoadedIndices.size();

			std::vector<objl::Vertex> vertex_buffer;
			for (int i = 0; i < mesh_count; ++i) {
				auto curr_mesh = loader.LoadedMeshes[i];
				for (int j = 0; j < curr_mesh.Vertices.size(); ++j) {
					vertex_buffer.push_back(curr_mesh.Vertices[j]);
				}

			}

			std::cout << "Done loading" << std::endl;
			glNamedBufferStorage(m_vertex_buffer, vertex_buffer.size() * sizeof(objl::Vertex), &vertex_buffer[0], 0);
			glVertexArrayVertexBuffer(m_vao, 0, m_vertex_buffer, 0, sizeof(objl::Vertex));

			glNamedBufferStorage(m_index_buffer, m_count * sizeof(GLuint), &loader.LoadedIndices[0], 0);
			glVertexArrayElementBuffer(m_vao, m_index_buffer);

			glVertexArrayAttribBinding(m_vao, 0, 0);
			glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(objl::Vertex, Position));
			glEnableVertexArrayAttrib(m_vao, 0);

			glVertexArrayAttribBinding(m_vao, 1, 0);
			glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(objl::Vertex, Normal));
			glEnableVertexArrayAttrib(m_vao, 1);

			glVertexArrayAttribBinding(m_vao, 2, 0);
			glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(objl::Vertex, TextureCoordinate));
			glEnableVertexArrayAttrib(m_vao, 2);


			glBindVertexArray(0);
		}
	}

	void OnInit(const char* filename) {
		m_program = LoadShaders(shader_text);
		objl::Loader loader;
		bool success = loader.LoadFile(filename);
		if (success) {

			for (int i = 0; i < loader.LoadedMeshes.size(); ++i) {

			}
			std::cout << "SUCCESS" << std::endl;
			int vert_count = loader.LoadedMeshes[0].Vertices.size();
			m_count = loader.LoadedIndices.size();

			std::cout << "Size of vertices: " << loader.LoadedMeshes[0].Vertices.size() * 8 * sizeof(float) << std::endl;
			std::cout << "Number of meshes: " << loader.LoadedMeshes.size() << std::endl;
			std::cout << "Number of indices: " << loader.LoadedIndices.size() << std::endl;
			
			glCreateVertexArrays(1, &m_vao);

			glCreateBuffers(1, &m_vertex_buffer);
			glNamedBufferStorage(m_vertex_buffer, vert_count * sizeof(objl::Vertex), &loader.LoadedMeshes[0].Vertices[0], 0);

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
			std::cout << "FAILURE" << std::endl;
		}
	}

	void OnUpdate(glm::mat4 mvp) {
		m_mvp = mvp;
	}

	void OnDraw() {
		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_mvp));
		glDrawElements(GL_TRIANGLES, m_count, GL_UNSIGNED_INT, (void*)0);
	}

	void OnDraw2() {
		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_mvp));
		glDrawElements(GL_TRIANGLES, m_count, GL_UNSIGNED_INT, (void*)0);
	}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	Mesh m_mesh;

	glm::vec3 m_cam_pos;
	glm::mat4 m_mvp;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0),
		m_cam_pos(-2.0f, 0.5f, 2.0f)
	{}

	void OnInit(Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");
		glEnable(GL_DEPTH_TEST);

		m_mesh.OnInit2("./resources/sponza.obj");

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		glm::mat4 lookat = glm::lookAt(glm::vec3(m_cam_pos.x, m_cam_pos.y, m_cam_pos.z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 perspective = glm::perspective(90.0f, 16.0f / 9.0f, 0.1f, 10000.0f);
		m_mvp = perspective * lookat;


		m_mesh.OnUpdate(m_mvp);
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		m_mesh.OnDraw2();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::InputFloat3("Cam Pos", glm::value_ptr(m_cam_pos));
		ImGui::End();
	}
};

SystemConf config = {
		1600,					//width
		900,					//height
		300,					//Position x
		200,					//Position y
		"Application",			//window title
		false,					//windowed fullscreen
		false,					//vsync
		144,					//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //TEST
