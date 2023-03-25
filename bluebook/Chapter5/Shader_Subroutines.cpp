#include "Defines.h"
#ifdef SHADER_SUBROUTINES
#include "System.h"
#include "Texture.h"

#include <string>
#include <sstream>

static GLfloat size = 0.5f;

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) uniform float u_size;

out vec2 vs_uv;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
	gl_Position = vec4(position * u_size, 1.0);
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

subroutine vec3 uv_subroutine(vec2 uv, float index);

layout (index = 0)
subroutine (uv_subroutine)
vec3 uv_function_one(vec2 uv, float index)
{
	return vec3(uv.x, uv.y, index);
}

layout (index = 1)
subroutine (uv_subroutine)
vec3 uv_function_two(vec2 uv, float index)
{
	return vec3(uv.y, uv.x, index);
}

subroutine uniform uv_subroutine u_sub;

out vec4 color;
in vec2 vs_uv;

layout (location = 3) uniform float u_index;
uniform sampler2DArray u_sampler;

void main() 
{
	vec3 uv = u_sub(vs_uv, u_index);
	color = texture(u_sampler, uv);
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
	-1.0f, -1.0f, 0.5f, 0.0f, 1.0f,
	-1.0f, 1.0f, 0.5f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.5f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.5f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.5f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.5f, 1.0f, 1.0f,
	};

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vertex_buffer);

	glNamedBufferStorage(vertex_buffer, sizeof(vertex_array), &vertex_array[0], 0);

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(vao, 0);

	glVertexArrayAttribBinding(vao, 1, 0);
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
	glEnableVertexArrayAttrib(vao, 1);

	glVertexArrayVertexBuffer(vao, 0, vertex_buffer, 0, sizeof(float) * 5);
	glBindVertexArray(0);

	return vao;
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	f32 m_index;

	GLuint m_program;
	GLuint m_vao;
	GLuint m_texture;
	GLuint m_subroutines[2];
	int m_active_sub_uniforms;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0),
		m_index(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);

		m_vao = GenerateQuad();

		const char* filenames[] = {
			"./resources/texture_array_2d/test_grid.ktx",
			"./resources/texture_array_2d/test_grid2.ktx",
			"./resources/texture_array_2d/test_grid3.ktx"
		};

		m_texture = CreateTextureArray(filenames, 3);

		

		glGetProgramStageiv(m_program, GL_FRAGMENT_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &m_active_sub_uniforms);
		m_subroutines[0] = glGetProgramResourceIndex(m_program, GL_FRAGMENT_SUBROUTINE, "uv_function_one");
		m_subroutines[1] = glGetProgramResourceIndex(m_program, GL_FRAGMENT_SUBROUTINE, "uv_function_two");
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();


	}
	void OnDraw() {
		int time = (int)m_time;

		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program);
		
		glBindVertexArray(m_vao);
		glProgramUniform1f(m_program, 2, size);
		glProgramUniform1f(m_program, 3, m_index);
		glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, m_active_sub_uniforms, &m_subroutines[time & 1]);
		glDrawArrays(GL_TRIANGLES, 0, 18);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::SliderFloat("Size", &size, 0.1, 1.0);
		ImGui::SliderFloat("Index", &m_index, 0.0, 5.0);
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
