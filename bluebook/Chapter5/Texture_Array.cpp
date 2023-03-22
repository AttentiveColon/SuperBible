#include "Defines.h"
#ifdef TEXTURE_ARRAY
#include "System.h"
#include "Texture.h"

static GLfloat size = 0.5f;

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

layout (location = 2) uniform float u_size;

out vec2 vs_uv;

void main() 
{
	gl_Position = vec4(position * u_size, 1.0);
	//gl_Position = vec4(position, 1.0);
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

layout (location = 3) uniform float u_time;
layout (location = 4) uniform float u_index;
uniform sampler2DArray u_texture;
//uniform sampler2D u_texture;

in vec2 vs_uv;

out vec4 color;

void main() 
{
	color = texture(u_texture, vec3(vs_uv, u_index));
	//color = texture(u_texture, vs_uv);
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
	GLuint m_vertex_buffer;
	GLuint m_texture;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0),
		m_index(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		//audio.PlayOneShot("./resources/startup.mp3");
		m_program = LoadShaders(shader_text);
		m_vao = GenerateQuad();

		/*const char* filenames[] = {
			"./resources/texture_array_2d/red.ktx",
			"./resources/texture_array_2d/blue.ktx",
			"./resources/texture_array_2d/green.ktx",
			"./resources/texture_array_2d/yellow.ktx",
			"./resources/texture_array_2d/purple.ktx"
		};*/

		const char* filenames[] = {
			"./resources/texture_array_2d/test_grid.ktx",
			"./resources/texture_array_2d/test_grid2.ktx",
			"./resources/texture_array_2d/test_grid3.ktx"
		};

		m_texture = CreateTextureArray(filenames, 3);

		//m_texture = Load_KTX("./resources/texture_array_2d/green.ktx");
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);

		//Bind size uniform
		glUniform1f(2, size);
		glUniform1f(3, (float)m_time);
		glUniform1f(4, m_index);
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
