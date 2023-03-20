#include "Defines.h"
#ifdef IMAGE2D
#include "System.h"

static const GLfloat vertices[] = {
	-1.0f, -1.0f, 0.5f, 0.0f, 0.0f,
	1.0f, -1.0f, 0.5f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.5f, 0.0f, 1.0f,
	-1.0f, 1.0f, 0.5f, 0.0f, 1.0f,
	1.0f, -1.0f, 0.5f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.5f, 1.0f, 1.0f
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

layout (location = 3) uniform float u_time;

layout (binding = 1, rgba32f) readonly uniform image2D u_image;
layout (binding = 2) uniform writeonly image2D u_image_out;

in vec2 vs_uv;

out vec4 color;

void main() 
{
	ivec2 P = ivec2(vs_uv);
	vec4 data = imageLoad(u_image, P);
	imageStore(u_image_out, P, data.bgra);
	color = data;
}
)";

static const GLchar* vertex_shader_source2 = R"(
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

static const GLchar* fragment_shader_source2 = R"(
#version 450 core

layout (location = 3) uniform float u_time;

layout (binding = 2, rgba32f) readonly uniform image2D u_image;


in vec2 vs_uv;

out vec4 color;

void main() 
{
	ivec2 P = ivec2(vs_uv);
	vec4 data = imageLoad(u_image, P);
	color = data;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static ShaderText shader_text2[] = {
	{GL_VERTEX_SHADER, vertex_shader_source2, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source2, NULL},
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
	GLuint m_program2;

	GLuint m_vao;
	GLuint m_vertex_buffer;

	GLuint m_image;
	GLuint m_image_out;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		//audio.PlayOneShot("./resources/startup.mp3");
		m_program = LoadShaders(shader_text);
		m_program2 = LoadShaders(shader_text2);
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vertex_buffer);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_image);
		glTextureStorage2D(m_image, 1, GL_RGBA32F, 1600, 900);
		glBindTexture(GL_TEXTURE_2D, m_image);
		float* data = new float[1600 * 900 * 4];
		GenerateTexture(data, 1600 * 900 * 4, 0xFF0000FF);
		glTextureSubImage2D(m_image, 0, 0, 0, 1600, 900, GL_RGBA, GL_FLOAT, data);
		delete[] data;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_image_out);
		glTextureStorage2D(m_image_out, 1, GL_RGBA32F, 1600, 900);

		glBindImageTexture(1, m_image, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(2, m_image_out, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		
		
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
		glUniform1f(3, (float)m_time);
		glDrawArrays(GL_TRIANGLES, 0, 18);
		
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program2);
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		//Bind size uniform
		glUniform1f(2, size);
		glUniform1f(3, (float)m_time);
		glBindTexture(GL_TEXTURE_2D, m_image_out);
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
