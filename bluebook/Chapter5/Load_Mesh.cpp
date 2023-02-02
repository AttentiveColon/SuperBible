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
	std::vector<GLuint> m_vaos;
	std::vector<GLuint> m_vertex_buffers;
	std::vector<GLuint> m_index_buffers;
	std::vector<GLuint> m_counts;
	GLuint m_vao;
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

			std::cout << "Mesh Count: " << mesh_count << std::endl;

			m_vaos.resize(mesh_count);
			m_vertex_buffers.resize(mesh_count);
			m_index_buffers.resize(mesh_count);
			m_counts.resize(mesh_count);

			glCreateVertexArrays(mesh_count, &m_vaos[0]);
			glCreateBuffers(mesh_count, &m_vertex_buffers[0]);
			glCreateBuffers(mesh_count, &m_index_buffers[0]);
			std::cout << "Loaded vert size: " << loader.LoadedVertices.size() << std::endl;
			std::cout << "Loaded index size: " << loader.LoadedIndices.size() << std::endl;

			for (int i = 0; i < mesh_count; ++i) {
				auto curr_mesh = loader.LoadedMeshes[i];
				m_counts[i] = curr_mesh.Vertices.size();

				std::cout << "Vertices size: " << curr_mesh.Vertices.size() << std::endl;
				std::cout << "Indices size: " << curr_mesh.Indices.size() << std::endl;

				
				glNamedBufferStorage(m_vertex_buffers[i], curr_mesh.Vertices.size() * sizeof(objl::Vertex), &curr_mesh.Vertices[i], 0);
				glVertexArrayVertexBuffer(m_vaos[i], 0, m_vertex_buffers[i], 0, sizeof(objl::Vertex));

				glNamedBufferStorage(m_index_buffers[i], curr_mesh.Indices.size() * sizeof(GLuint), &curr_mesh.Indices[i], 0);
				glVertexArrayElementBuffer(m_vaos[i], m_index_buffers[i]);

				glVertexArrayAttribBinding(m_vaos[i], 0, 0);
				glVertexArrayAttribFormat(m_vaos[i], 0, 3, GL_FLOAT, GL_FALSE, offsetof(objl::Vertex, Position));
				glEnableVertexArrayAttrib(m_vaos[i], 0);

				glVertexArrayAttribBinding(m_vaos[i], 1, 0);
				glVertexArrayAttribFormat(m_vaos[i], 1, 3, GL_FLOAT, GL_FALSE, offsetof(objl::Vertex, Normal));
				glEnableVertexArrayAttrib(m_vaos[i], 1);

				glVertexArrayAttribBinding(m_vaos[i], 2, 0);
				glVertexArrayAttribFormat(m_vaos[i], 2, 2, GL_FLOAT, GL_FALSE, offsetof(objl::Vertex, TextureCoordinate));
				glEnableVertexArrayAttrib(m_vaos[i], 2);


				glBindVertexArray(0);
			}

			std::cout << "Done loading" << std::endl;
			
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
			m_count = loader.LoadedIndices.size() * 3;

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
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_mvp));

		for (int i = 0; i < m_vaos.size(); ++i) {
			glBindVertexArray(m_vaos[i]);
			//glDrawElements(GL_TRIANGLES, m_counts[i], GL_UNSIGNED_INT, (void*)0);
			glDrawArrays(GL_TRIANGLES, 0, m_counts[i]);
		}
	}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	Mesh m_mesh;

	glm::vec3 m_cam_pos;
	glm::vec3 m_direction;
	GLfloat m_angle_x;
	GLfloat m_angle_y;
	glm::mat4 m_mvp;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0),
		m_cam_pos(-2.0f, 0.5f, 2.0f),
		m_direction(0.0f, 0.0f, 0.0f),
		m_angle_x(0.0),
		m_angle_y(0.0)
	{
		m_direction = glm::normalize(m_direction - m_cam_pos);
	}

	void OnInit(Input& input, Audio& audio, Window& window) {
		input.SetRawMouseMode(window.GetHandle());
		audio.PlayOneShot("./resources/startup.mp3");
		glEnable(GL_DEPTH_TEST);

		m_mesh.OnInit2("./resources/basic_scene.obj");

		

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		MousePos mouse_pos = input.GetMouseRaw();
		m_angle_x += mouse_pos.x;
		m_angle_y = glm::clamp(m_angle_y + mouse_pos.y, -45.0, 45.0);

		glm::vec3 front;
		front.x = cos(glm::radians(m_angle_x) * cos(glm::radians(0.0)));
		front.y = -sin(glm::radians(m_angle_y));
		front.z = sin(glm::radians(m_angle_x) * cos(glm::radians(0.0)));
		m_direction = front + m_cam_pos;


		if (input.Held(GLFW_KEY_W)) {
			m_cam_pos += glm::normalize(front) * (f32)dt * 2.0f;
		}
		if (input.Held(GLFW_KEY_S)) {
			m_cam_pos += -glm::normalize(front) * (f32)dt * 2.0f;
		}


		glm::mat4 lookat = glm::lookAt(m_cam_pos, m_direction, glm::vec3(0.0f, 1.0f, 0.0f));
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
		ImGui::DragFloat3("Cam Pos", glm::value_ptr(m_cam_pos));
		ImGui::Text("AngleX: %f", m_angle_x);
		ImGui::Text("AngleY: %f", m_angle_y);
		//ImGui::InputFloat3("Cam Pos", glm::value_ptr(m_cam_pos));
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
