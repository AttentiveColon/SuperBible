#include "Defines.h"
#ifdef USER_DEFINED_CLIPPING_PLANES
#include "System.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

void main() 
{
	gl_Position = vec4(0.0, 0.0, 0.5, 1.0);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main() 
{
	color = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_vao;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		m_program = LoadShaders(shader_text);
		glCreateVertexArrays(1, &m_vao);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_POINTS, 0, 1);
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
#endif //USER_DEFINED_CLIPPING_PLANES
