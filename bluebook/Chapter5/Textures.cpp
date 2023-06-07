#include "Defines.h"
#ifdef TEXTURES
#include "System.h"

static const GLfloat vertices[] = {
	-1.0f, -1.0f, 0.5f,
	1.0f, -1.0f, 0.5f, 
	-1.0f, 1.0f, 0.5f,
	-1.0f, 1.0f, 0.5f,
	1.0f, -1.0f, 0.5f,
	1.0f, 1.0f, 0.5f
};

static GLfloat resolution[] = {
	1600.0, 900.0
};

static GLfloat size = 0.5f;

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 2) uniform float u_size;
 

void main() 
{
	gl_Position = vec4(position * u_size, 1.0);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

layout (location = 1) uniform vec2 u_resolution;

uniform sampler2D texture;

out vec4 color;

void main() 
{
	color = texelFetch(texture, ivec2(gl_FragCoord.xy), 0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

void generate_texture(float* data, u32 width, u32 height) {
	int strip_width = 100;
	for (int i = 0; i < width * height * 4; i += 4) {
		int value = i % strip_width;
		if (value < strip_width / 2) {
			data[i] = 0.0f;
			data[i + 1] = 0.0f;
			data[i + 2] = 0.0f;
			data[i + 3] = 1.0f;
		}
		else {
			data[i] = 0.5f;
			data[i + 1] = 0.5f;
			data[i + 2] = 0.5f;
			data[i + 3] = 1.0f;
		}
	}
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	GLuint m_program;
	GLuint m_program2;
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

		generate_texture(data, 1600, 900);
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
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		//Bind resolution uniform
		glUniform2fv(1, 1, resolution);
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
