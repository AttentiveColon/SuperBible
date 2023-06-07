#include "Defines.h"
#ifdef ATOMIC_COUNTER_BUFFER
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
layout (binding = 0, offset = 0) uniform atomic_uint counter;

//out vec4 color;

void main() 
{
	atomicCounterIncrement(counter);
	//color = vec4(gl_FragCoord.y / u_resolution.y, gl_FragCoord.x / u_resolution.x, 0.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* vertex_shader_source2 = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 2) uniform float u_size;
 

void main() 
{
	gl_Position = vec4(position * u_size, 1.0);
}
)";

static const GLchar* fragment_shader_source2 = R"(
#version 450 core

layout (location = 1) uniform vec2 u_resolution;
layout (location = 3) uniform uint counter;

out vec4 color;

void main() 
{
	float brightness = clamp(float(counter) / (u_resolution.x * u_resolution.y), 0.0, 1.0);
	color = vec4(brightness, brightness, brightness, 1.0);
}
)";

static ShaderText shader_text2[] = {
	{GL_VERTEX_SHADER, vertex_shader_source2, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source2, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	GLuint m_program;
	GLuint m_program2;
	GLuint m_vao;
	GLuint m_vertex_buffer;

	GLuint m_atomic_counter;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {

		m_program = LoadShaders(shader_text);
		m_program2 = LoadShaders(shader_text2);
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vertex_buffer);

		//Generate atomic counter
		glGenBuffers(1, &m_atomic_counter);
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

		//Bind to GL_ATOMIC_COUNTER_BUFFER target and initialize its storage
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomic_counter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_COPY);
		//Reset atomic buffer to known value every frame
		const GLuint zero = 0;
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);
		//Now bind it to shader binding point
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomic_counter);
		//Bind resolution uniform
		glUniform2fv(1, 1, resolution);
		//Bind size uniform
		glUniform1f(2, size);
		glDrawArrays(GL_TRIANGLES, 0, 18);

		GLuint count;
		glGetNamedBufferSubData(m_atomic_counter, 0, sizeof(GLuint), &count);
		
		glUseProgram(m_program2);
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glUniform2fv(1, 1, resolution);
		glUniform1f(2, size);
		glUniform1ui(3, count);
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
