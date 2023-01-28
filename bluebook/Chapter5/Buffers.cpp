#include "Defines.h"
#ifdef BUFFERS
#include "System.h"

static const float data[] =
{
	0.25, -0.25, 0.5, 1.0,
	-0.25, -0.25, 0.5, 1.0,
	0.25, 0.25, 0.5, 1.0
};

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
	color = vec4(0.0, 0.8, 1.0, 1.0);
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
	
	GLuint m_buffer;
	GLuint m_program;

	GLuint VAO;

	

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		m_buffer(0),
		m_program(0),
		VAO(0)
	{}

	void OnInit(Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");

		m_program = LoadShaders(shader_text);

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		glCreateBuffers(1, &m_buffer);
		glNamedBufferStorage(m_buffer, sizeof(data), NULL, GL_MAP_WRITE_BIT);
		void* ptr = glMapNamedBufferRange(m_buffer, 0, sizeof(data), GL_MAP_WRITE_BIT);
		memcpy(ptr, data, sizeof(1));
		glUnmapNamedBuffer(m_buffer);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);

		glUseProgram(m_program);

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
#endif //TEST
