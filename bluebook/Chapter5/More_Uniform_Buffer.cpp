#include "Defines.h"
#ifdef MORE_UNIFORM_BUFFER
#include "System.h"
#include "Model.h"
#include "Texture.h"

static GLfloat size = 0.5f;

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

layout (binding = 0)
uniform Global
{
	vec2 u_resolution;
	float u_time;
	float u_index;
};

out vec2 vs_uv;


void main() 
{
	vec3 pos = position + (vec3(0.2, 0.2, 0.0) * sin(u_time * 0.25));
	gl_Position = vec4(pos, 1.0);
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

uniform sampler2DArray u_texture;

layout (binding = 0, std140)
uniform Global
{
	vec2 u_resolution;
	float u_time;
	float u_index;
};

in vec2 vs_uv;

out vec4 color;

void main() 
{
	color = texture(u_texture, vec3(vs_uv, u_index));
	vec2 st = gl_FragCoord.xy / u_resolution;
	float y = st.x * 0.8;
	vec3 c = vec3(y);
	color += vec4(c, 1.0);
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

struct UBO {
	float u_resolution_width;
	float u_resolution_height;
	float u_time;
	float u_index;
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	float m_time;
	vec2 m_resolution;

	f32 m_index;

	GLuint m_program;
	GLuint m_vao;
	GLuint m_vertex_buffer;
	GLuint m_texture;

	GLuint m_ubo;
	char* m_ubo_data;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0),
		m_index(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {

		m_program = LoadShaders(shader_text);
		m_vao = GenerateQuad();

		glCreateBuffers(1, &m_ubo);
		m_ubo_data = new char[48];



		const char* filenames[] = {
			"./resources/texture_array_2d/test_grid.ktx",
			"./resources/texture_array_2d/test_grid2.ktx",
			"./resources/texture_array_2d/test_grid3.ktx"
		};

		m_texture = CreateTextureArray(filenames, 3);

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = (float)window.GetTime();
		m_resolution = vec2(window.GetWindowDimensions().width, window.GetWindowDimensions().height);

		UBO ubo = { m_resolution.x, m_resolution.y, m_time, m_index };

		memcpy(m_ubo_data, &ubo, sizeof(UBO));
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);

		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, 32, m_ubo_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);

		glDrawArrays(GL_TRIANGLES, 0, 18);

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::SliderFloat("Size", &size, 0.1, 1.0);
		ImGui::SliderFloat("Index", &m_index, 0.0, 4.0);
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
