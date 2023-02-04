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

layout (location = 4) uniform mat4 mvp;
layout (location = 5) uniform float u_time;
layout (location = 6) uniform vec2 u_resolution;

uniform UniformBlock
{
	float u_time;
	vec2 u_resolution;
	mat4 mvp;
} u_block;

out vec3 vs_normal;
out vec2 vs_uv;

void main() 
{
	vec4 new_pos = mvp * vec4(position.x, position.y, position.z, 1.0);
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
	color = vec4(0.5 * (vs_normal.y * vs_normal.x + 0.1) + 0.1, 0.5 * (vs_normal.y * (vs_normal.z + 0.2)) + 0.1, 0.5 * (vs_normal.y * (vs_normal.x + 0.3)) + 0.1, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Mesh {
	GLuint m_vao;
	GLuint m_vertex_buffer;
	GLuint m_index_buffer;
	GLsizei m_count;
	GLuint m_program;

	glm::mat4 m_mvp;
	f64 m_time;
	WindowSize m_resolution;

	void Load_OBJ(const char* filename) {
		m_program = LoadShaders(shader_text);
		objl::Loader loader;
		bool success = loader.LoadFile(filename);
		if (success) {
			int vert_count = loader.LoadedVertices.size();
			m_count = loader.LoadedIndices.size() * 3;
			
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

	void OnUpdate(glm::mat4 mvp, f64 time, WindowSize resolution) {
		m_mvp = mvp;
		m_time = time;
		m_resolution = resolution;
	}

	//associate mvp, m_time and window resolution in uniform array
	void OnDraw() {
		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(m_mvp));
		glUniform1f(5, (float)m_time);
		glUniform2i(6, m_resolution.width, m_resolution.height);
		glDrawElements(GL_TRIANGLES, m_count, GL_UNSIGNED_INT, (void*)0);
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
		input.SetRawMouseMode(window.GetHandle(), true);
		audio.PlayOneShot("./resources/startup.mp3");
		glEnable(GL_DEPTH_TEST);

		m_mesh.Load_OBJ("./resources/basic_scene.obj");

		

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (input.Pressed(GLFW_KEY_BACKSPACE)) {
			if (input.IsMouseRawActive()) {
				input.SetRawMouseMode(window.GetHandle(), false);
			}
			else {
				input.SetRawMouseMode(window.GetHandle(), true);
			}
		}
		
		if (input.IsMouseRawActive()) {
			MousePos mouse_pos = input.GetMouseRaw();
			m_angle_x += mouse_pos.x;
			m_angle_y = glm::clamp(m_angle_y + mouse_pos.y, -45.0, 45.0);
		}

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




		m_mesh.OnUpdate(m_mvp, m_time, window.GetWindowDimensions());
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		m_mesh.OnDraw();
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
