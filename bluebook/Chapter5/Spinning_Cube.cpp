#include "Defines.h"
#ifdef SPINNING_CUBE
#include "System.h"

#include "glm/common.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 3) uniform mat4 u_model;
layout (location = 4) uniform mat4 u_viewproj;

out vec4 vs_normal;
out vec2 vs_uv;
out vec4 vs_color;

void main() 
{
	gl_Position = u_viewproj * u_model * vec4(position, 1.0);
	vs_normal = u_model * vec4(normal, 1.0);
	vs_uv = uv;
	vs_color = vec4(position, 1.0) * 0.5 + vec4(0.02, 0.1, 0.1, 0.0);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in vec4 vs_normal;
in vec2 vs_uv;
in vec4 vs_color;

out vec4 color;

void main() 
{
	color = vs_color + vec4(0.0, 0.0, 0.5, 1.0);
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
	ObjMesh m_mesh;

	glm::mat4 m_model;
	glm::mat4 m_viewproj;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");
		glEnable(GL_DEPTH_TEST);

		m_program = LoadShaders(shader_text);
		m_mesh.Load_OBJ("./resources/cube.obj");
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		m_viewproj = glm::perspective(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);
		m_viewproj *= glm::lookAt(glm::vec3(-10.0f, 15.0f, -20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program);
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(m_viewproj));

		for (int i = 0; i < 48; ++i) {
			float f = (float)i + m_time * 0.3f;
			glm::mat4 model = glm::mat4(1.0f);
			m_model = glm::rotate(model, glm::radians((float)m_time * 5.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
				glm::rotate(model, glm::radians((float)m_time * 2.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			m_model *= glm::translate(m_model, glm::vec3(sinf(2.1f * f) * 8.0f, cosf(1.7f * f) * 8.0f, sinf(1.3f * f) * cosf(1.5f * f) * 8.0f));
			glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(m_model));
			m_mesh.OnDraw();

		}


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
