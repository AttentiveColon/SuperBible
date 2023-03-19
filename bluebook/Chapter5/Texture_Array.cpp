#include "Defines.h"
#ifdef TEXTURE_ARRAY
#include "System.h"

static const GLfloat vertices[] = {
	-1.0f, -1.0f, 0.5f, 0.0f, 0.0f,
	1.0f, -1.0f, 0.5f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.5f, 0.0f, 1.0f,
	-1.0f, 1.0f, 0.5f, 0.0f, 1.0f,
	1.0f, -1.0f, 0.5f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.5f, 1.0f, 1.0f
};

static GLfloat resolution[] = {
	1600.0, 900.0
};

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
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core



uniform sampler2D u_texture;

in vec2 vs_uv;

out vec4 color;

void main() 
{
	color = texture(u_texture, vs_uv);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

void GenerateTexture(float* data, size_t size, int color) {
	float color_r = (float)(color & 0xFF000000) / 255.0f;
	float color_g = (float)(color & 0x00FF0000) / 255.0f;
	float color_b = (float)(color & 0x0000FF00) / 255.0f;
	float color_a = (float)(color & 0x000000FF) / 255.0f;
	for (size_t i = 0; i < size; i += 4) {
		data[i] = color_r;
		data[i + 1] = color_g;
		data[i + 2] = color_b;
		data[i + 3] = color_a;
	}
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	GLuint m_program;

	GLuint m_vao;
	GLuint m_vertex_buffer;

	GLuint m_texture;
	GLfloat* data;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		//audio.PlayOneShot("./resources/startup.mp3");
		m_program = LoadShaders(shader_text);
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vertex_buffer);

		//Create texture buffer
		glCreateTextures(GL_TEXTURE_2D, 1, &m_texture);
		//Specify storage size
		glTextureStorage2D(m_texture, 1, GL_RGBA32F, 1600, 900);
		//Bind the texture
		glBindTexture(GL_TEXTURE_2D, m_texture);

		//Create the texture data
		data = new float[1600 * 900 * 4];

		GenerateTexture(data, 1600 * 900 * 4, 0x0000FFFF);
		glTextureSubImage2D(m_texture, 0, 0, 0, 1600, 900, GL_RGBA, GL_FLOAT, data);
		delete[] data;
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		
		glEnableVertexAttribArray(1);
		//Bind size uniform
		glUniform1f(2, size);
		glDrawArrays(GL_TRIANGLES, 0, 18);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::SliderFloat("Size", &size, 0.1, 1.0);
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
		30,					//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //TEST
