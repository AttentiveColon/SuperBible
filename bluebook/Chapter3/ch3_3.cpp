#include "Defines.h"
#ifdef CH3_3
#include "System.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec4 offset;

void main() 
{
	const vec4 vertices[3] = vec4[3] (vec4(0.25, -0.25, 0.5, 1.0),
									  vec4(-0.25, -0.25, 0.5, 1.0),
									  vec4(0.25, 0.25, 0.5, 1.0));

	gl_Position = vertices[gl_VertexID] + offset;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main() 
{
	color = vec4(sin(gl_FragCoord.x * 0.25) * 0.5 + 0.5,
				 cos(gl_FragCoord.y * 0.25) * 0.5 + 0.5,
				 sin(gl_FragCoord.x * 0.15) * cos(gl_FragCoord.y * 0.15),
				 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	GLfloat m_offset[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_vertex_array_object;

	Application()
		:m_clear_color{ 0.0f, 0.2f, 0.0f, 1.0f },
		m_offset{ static_cast<float>(sin(m_time) * 0.5f), static_cast<float>(cos(m_time) * 0.6f), 0.0f, 0.0f },
		m_fps(0),
		m_time(0),
		m_program(NULL),
		m_vertex_array_object(NULL)
	{}

	~Application() {
		glDeleteVertexArrays(1, &m_vertex_array_object);
		glDeleteProgram(m_program);
	}

	void OnInit(Input& input, Audio& audio, Window& window) {

		m_program = LoadShaders(shader_text);
		glCreateVertexArrays(1, &m_vertex_array_object);
		glBindVertexArray(m_vertex_array_object);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
		m_clear_color[0] = static_cast<float>(sin(m_time) * 0.5 + 0.5);
		m_clear_color[1] = static_cast<float>(cos(m_time) * 0.5 + 0.5);
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program);
		glVertexAttrib4fv(0, m_offset);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::SliderFloat("Offset X", &m_offset[0], -1.0, 1.0);
		ImGui::SliderFloat("Offset Y", &m_offset[1], -1.0, 1.0);
		ImGui::End();
	}
};

SystemConf config = {
		1600,					//width
		900,					//height
		300,					//Position x
		200,					//Position y
		"Chapter_3_3",			//window title
		false,					//windowed fullscreen
		false,					//vsync
		144,					//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //CH3_3