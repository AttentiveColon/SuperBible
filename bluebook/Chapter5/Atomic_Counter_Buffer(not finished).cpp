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

static const GLfloat resolution[] = {
	1600.0, 900.0
};

static GLint storage_block[] = {
	0, 0
};

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;

layout (binding = 0, std430) buffer storage_block
{
	int number;
	int number2;
};



void main() 
{
	atomicAdd(number, 1);
	atomicAdd(number2, number);
	gl_Position = vec4(position * 0.5, 1.0);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

layout (location = 1) uniform vec2 u_resolution;

layout (binding = 0, std430) buffer storage_block
{
	int number;
	int number2;
};

out vec4 color;

void main() 
{
	float coordY = mod(gl_FragCoord.y, 4);
	color = vec4(coordY / u_resolution.y, gl_FragCoord.x / u_resolution.x, 0.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	GLuint m_program;
	GLuint m_vao;
	GLuint m_vertex_buffer;
	GLuint m_storage_buffer;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");
		m_program = LoadShaders(shader_text);
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vertex_buffer);

		//Generate storage buffer name
		glGenBuffers(1, &m_storage_buffer);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		glClearBufferfv(GL_COLOR, 0, m_clear_color);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glUniform2fv(1, 1, resolution);

		//Bind the storage buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		//Pass the data to the buffer
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(storage_block), storage_block, GL_STREAM_READ);
		//glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(storage_block), storage_block, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		//Bind the data to binding point 0 in shader
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		//Unbind the buffer
		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glDrawArrays(GL_TRIANGLES, 0, 18);

		//Map the storage buffer and check the values were modified in the shader
		GLint array[2];
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(storage_block), &array);
		
		//GLint* array = (GLint*)glMapNamedBuffer(m_storage_buffer, GL_READ_ONLY);
		std::cout << &array << std::endl;
		std::cout << "number one:" << array[0] << std::endl;
		std::cout << "number two:" << array[1] << std::endl;
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
		5,					//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //TEST
