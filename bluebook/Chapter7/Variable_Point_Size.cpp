#include "Defines.h"
#ifdef VARIABLE_POINT_SIZE
#include "System.h"
#include "Texture.h"
#include "Model.h"

#include "glm/common.hpp"	
#include "glm/glm.hpp"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) uniform vec3 eye;
layout (location = 2) uniform mat4 viewproj;

void main() 
{
	const float a = 0.0f;
	const float b = 0.0f;
	const float c = 50.0f;

	float d = distance(position, eye);
	float size = 500.0 * sqrt(1.0f / (a + b * d + c * pow(d, 2)));

	gl_PointSize = size;
	gl_Position = viewproj * vec4(position, 1.0);
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

static GLuint GenerateQuad() {
	GLuint vao, vertex_buffer;

	static const GLfloat vertex_array[] = {
	-0.5f, -0.5f, 0.5f,
	-0.5f, 0.5f, 0.5f,
	0.5f, 0.5f, 0.5f,
	0.5f, -0.5f, 0.5f,
	};

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vertex_buffer);

	glNamedBufferStorage(vertex_buffer, sizeof(vertex_array), &vertex_array[0], 0);

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(vao, 0);

	glVertexArrayVertexBuffer(vao, 0, vertex_buffer, 0, sizeof(float) * 3);


	glBindVertexArray(0);

	return vao;
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	SB::Camera m_camera;

	glm::mat4 m_viewproj;
	glm::vec3 m_eye = vec3(0.0f, 0.0f, -5.0f);

	GLuint m_program;
	GLuint m_vao;
	GLfloat m_point_size = 5.0f;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{
		std::string name = "camera";
		m_camera = SB::Camera(name, vec3(0.0f, 0.0f, -5.0f), vec3(0.0f, 0.0f, 0.0f), SB::Perspective, 1.0 / 1.0f, 90.0f, 0.001f, 100.0f);
	}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);

		m_vao = GenerateQuad();

		glEnable(GL_PROGRAM_POINT_SIZE);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
		
		if (input.Held(GLFW_KEY_LEFT_CONTROL)) {
			input.SetRawMouseMode(window.GetHandle(), false);
		}
		else {
			input.SetRawMouseMode(window.GetHandle(), true);
			m_camera.OnUpdate(input, 3.0f, 2.5f, dt);
		}

		m_viewproj = m_camera.ViewProj();
		m_eye = vec3(m_camera.m_view[3][0], m_camera.m_view[3][1], m_camera.m_view[3][2]);
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program);
		glUniform3fv(1, 1, value_ptr(m_eye));
		glUniformMatrix4fv(2, 1, GL_FALSE, value_ptr(m_viewproj));
		glBindVertexArray(m_vao);
		glDrawArrays(GL_POINTS, 0, 4);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", (int)m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat("PointSize", &m_point_size, 0.1f, 1.0f, 50.0f);
		ImGui::Text("Eye: %f %f %f", m_eye.x, m_eye.y, m_eye.z);
		ImGui::End();
	}
};

SystemConf config = {
		900,					//width
		900,					//height
		300,					//Position x
		200,					//Position y
		"Application",			//window title
		false,					//windowed fullscreen
		false,					//vsync
		30,					//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //TEST
