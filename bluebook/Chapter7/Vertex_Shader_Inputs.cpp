#include "Defines.h"
#ifdef VERTEX_SHADER_INPUTS
#include "System.h"
#include "Texture.h"

#include "glm/common.hpp"	
#include "glm/glm.hpp"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 color;
layout (location = 4) in float lightness;

out vec2 vs_uv;
out vec4 vs_color;
out float vs_lightness;

void main() 
{
	gl_Position = vec4(position, 1.0);
	vs_uv = uv;
	vs_color = color;
	vs_lightness = lightness;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in vec2 vs_uv;
in vec4 vs_color;
in float vs_lightness;

out vec4 color;

uniform sampler2D u_sampler;

void main() 
{
	color = texture(u_sampler, vs_uv);
	color *= vs_color;
	color *= vec4(vs_lightness);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Vertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec4 color;
	int material_id;
};

static GLuint GenerateQuad() {
	GLuint vao, vertex_buffer, material_buffer;

	static const GLfloat vertex_array[] = {
	-1.0f, -1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-1.0f, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};

	static const GLfloat material_array[] = {
		0.1f, 0.1f, 1.5f, 0.1f, 1.5f, 1.5f
	};

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vertex_buffer);
	glCreateBuffers(1, &material_buffer);

	glNamedBufferStorage(vertex_buffer, sizeof(vertex_array), &vertex_array[0], 0);
	glNamedBufferStorage(material_buffer, sizeof(material_array), &material_array[0], 0);

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
	glEnableVertexArrayAttrib(vao, 0);

	glVertexArrayAttribBinding(vao, 1, 0);
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
	glEnableVertexArrayAttrib(vao, 1);

	glVertexArrayAttribBinding(vao, 2, 0);
	glVertexArrayAttribFormat(vao, 2, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, color));
	glEnableVertexArrayAttrib(vao, 2);

	glVertexArrayAttribBinding(vao, 4, 1);
	glVertexArrayAttribFormat(vao, 4, 1, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(vao, 4);

	glVertexArrayVertexBuffer(vao, 0, vertex_buffer, 0, sizeof(float) * 9);
	glVertexArrayVertexBuffer(vao, 1, material_buffer, 0, sizeof(float) * 1);
	
	glBindVertexArray(0);

	return vao;
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_vao;
	GLuint m_texture;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);

		m_vao = GenerateQuad();

		m_texture = Load_KTX("./resources/texture_array_2d/test_grid.ktx");
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLES, 0, 18);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", (int)m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
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
