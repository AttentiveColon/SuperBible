#include "Defines.h"
#ifdef BUFFERS
#include "System.h"

static const float mega_data[] =
{
	0.25, -0.25, 0.5, 1.0,	//vertex 1
	1.0, 0.0, 0.0, 1.0,		//color 1
	-0.25, -0.25, 0.5, 1.0,	//vertex 2
	0.0, 1.0, 0.0, 1.0,		//color 2
	0.25, 0.25, 0.5, 1.0,	//vertex 3
	0.0, 0.0, 1.0, 1.0		//color 3
};

static const float data[] =
{
	0.25, -0.25, 0.5, 1.0,
	-0.25, -0.25, 0.5, 1.0,
	0.25, 0.25, 0.5, 1.0
};

static const float color_data[] =
{
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0
};

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;

out vec4 vs_color;

void main() 
{
	gl_Position = position;
	vs_color = color;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in vec4 vs_color;

out vec4 color;

void main() 
{
	color = vs_color;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Triangle {
	GLuint VAO;
	GLuint m_vertex_buffer;
	GLuint m_color_buffer;
	GLuint m_program;

	Triangle() :VAO(0), m_vertex_buffer(0), m_color_buffer(0), m_program(0) {}

	void OnInit() {
		m_program = LoadShaders(shader_text);
		glCreateVertexArrays(1, &VAO);
		//glBindVertexArray(VAO);

		glCreateBuffers(1, &m_vertex_buffer);
		glNamedBufferStorage(m_vertex_buffer, sizeof(mega_data), &mega_data, GL_MAP_WRITE_BIT);
		glVertexArrayVertexBuffer(
			VAO,				//vaobj
			0,					//binding index
			m_vertex_buffer,	//buffer object
			0,					//offset
			sizeof(float) * 8	//stride
		);
		glVertexArrayAttribFormat(
			VAO,				//vaobj
			0,					//attribute index
			4,					//size
			GL_FLOAT,			//type
			GL_FALSE,			//normalized
			0					//relative offset
		);
		glVertexArrayAttribBinding(VAO, 0, 0);
		glEnableVertexArrayAttrib(VAO, 0);

		glCreateBuffers(1, &m_color_buffer);
		glNamedBufferStorage(m_color_buffer, sizeof(mega_data), &mega_data, GL_MAP_WRITE_BIT);
		glVertexArrayVertexBuffer(VAO, 1, m_color_buffer, 0, sizeof(float) * 8);
		glVertexArrayAttribFormat(VAO, 1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4);
		glVertexArrayAttribBinding(VAO, 1, 1);
		glEnableVertexArrayAttrib(VAO, 1);

		//Using two seperate data buffers

		//glCreateBuffers(1, &m_vertex_buffer);
		//glNamedBufferStorage(m_vertex_buffer, sizeof(data), &data, GL_MAP_WRITE_BIT);
		//glVertexArrayVertexBuffer(
		//	VAO,				//vaobj
		//	0,					//binding index
		//	m_vertex_buffer,	//buffer object
		//	0,					//offset
		//	sizeof(float) * 4	//stride
		//);
		//glVertexArrayAttribFormat(
		//	VAO,				//vaobj
		//	0,					//attribute index
		//	4,					//size
		//	GL_FLOAT,			//type
		//	GL_FALSE,			//normalized
		//	0					//relative offset
		//);
		//glVertexArrayAttribBinding(VAO, 0, 0);
		//glEnableVertexArrayAttrib(VAO, 0);

		//glCreateBuffers(1, &m_color_buffer);
		//glNamedBufferStorage(m_color_buffer, sizeof(color_data), &color_data, GL_MAP_WRITE_BIT);
		//glVertexArrayVertexBuffer(VAO, 1, m_color_buffer, 0, sizeof(float) * 4);
		//glVertexArrayAttribFormat(VAO, 1, 4, GL_FLOAT, GL_FALSE, 0);
		//glVertexArrayAttribBinding(VAO, 1, 1);
		//glEnableVertexArrayAttrib(VAO, 1);

	}

	void OnDraw() {
		glUseProgram(m_program);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	Triangle m_triangle;
	

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0),
		m_triangle()
	{}

	void OnInit(Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");

		m_triangle.OnInit();

		//mapping data to buffer example

		/*glCreateBuffers(1, &m_buffer);
		glNamedBufferStorage(m_buffer, sizeof(data), NULL, GL_MAP_WRITE_BIT);
		void* ptr = glMapNamedBufferRange(m_buffer, 0, sizeof(data), GL_MAP_WRITE_BIT);
		memcpy(ptr, data, sizeof(1));
		glUnmapNamedBuffer(m_buffer);*/

		
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);

		m_triangle.OnDraw();
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
