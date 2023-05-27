#include "Defines.h"
#ifdef PACKET_BUFFER
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include <chrono>
#include <omp.h>

static const GLchar* vs_source = R"(
#version 450 core

layout (binding = 0) uniform BLOCK
{
	vec4 vtx_color[4];
};

out vec4 vs_fs_color;

void main()
{
	vs_fs_color = vtx_color[gl_VertexID & 3];
	gl_Position = vec4((gl_VertexID & 2) - 1.0, (gl_VertexID & 1) * 2.0 - 1.0, 0.5, 1.0);
}
)";

static const GLchar* fs_source = R"(
#version 450 core

layout (location = 0) out vec4 color;

in vec4 vs_fs_color;

void main()
{
	color = vs_fs_color;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vs_source, NULL},
	{GL_FRAGMENT_SHADER, fs_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLfloat colors[] = {
	1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 1.0f, 1.0f,
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_vao;
	GLuint m_program;
	GLuint m_buffer_block;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		//Init Stream
		m_program = LoadShaders(shader_text);
		glGenVertexArrays(1, &m_vao);
		//Create Uniform buffer block

		

		glGenBuffers(1, &m_buffer_block);
		glBindBuffer(GL_UNIFORM_BUFFER, m_buffer_block);
		glBufferStorage(GL_UNIFORM_BUFFER, sizeof(colors), colors, 0);

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_buffer_block, 0, sizeof(colors));
		glDrawArrays(GL_TRIANGLES, 0, 12);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
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
#endif //PACKET_BUFFER