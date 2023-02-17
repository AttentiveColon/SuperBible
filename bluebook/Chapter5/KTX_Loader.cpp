#include "Defines.h"
#ifdef KTX_LOADER
#include "System.h"
#include "Texture.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core 

void main() 
{
	const vec4 vertices[3] = vec4[3] (vec4(-1.0, 1.0, 0.5, 1.0),
									  vec4(3.0, 1.0, 0.5, 1.0),
									  vec4(-1.0, -3.0, 0.5, 1.0));

	gl_Position = vertices[gl_VertexID];
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

uniform sampler2D u_texture;

out vec4 color;

void main() 
{
	color = texture(u_texture, gl_FragCoord.xy / ivec2(1600, -900));
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
	GLuint m_texture;
	

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");
		m_texture = Load_KTX("./resources/face1.ktx");
		m_program = LoadShaders(shader_text);
		glGenVertexArrays(1, &m_vao);

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glDrawArrays(GL_TRIANGLES, 0, 3);
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
