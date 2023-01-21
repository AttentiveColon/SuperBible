#include "Current_Project.h"
#ifdef CH2_2
#include "System.h"

GLuint compile_shaders();

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_vertex_array_object;
	GLfloat m_point_size;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		m_program(NULL),
		m_vertex_array_object(NULL),
		m_point_size(1.0f)
	{}

	~Application() {
		glDeleteVertexArrays(1, &m_vertex_array_object);
		glDeleteProgram(m_program);
	}

	void OnInit(Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");
		m_program = compile_shaders();
		glCreateVertexArrays(1, &m_vertex_array_object);
		glBindVertexArray(m_vertex_array_object);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		m_clear_color[0] = (float)sin(m_time) * 0.5 + 0.5;
		m_clear_color[1] = (float)cos(m_time) * 0.5 + 0.5;


	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glPointSize(m_point_size);
		glUseProgram(m_program);
		glDrawArrays(GL_POINTS, 0, 1);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat("Point size:", &m_point_size, 1.0f, 1.0f, 500.0f);
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
		144,						//framelimit
		"resources/Icon.bmp" //icon path
};

MAIN(config)

GLuint compile_shaders() {
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;

	static const GLchar* vertex_shader_source = R"(
#version 450 core

void main() 
{
	gl_Position = vec4(0.0, 0.0, 0.5, 1.0);
}
)";

	static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main() 
{
	color = vec4(0.0, 0.8, 1.0, 1.0);
}
)";

	//compile vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	//compile fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	//attach and link shaders
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	
	//clean up
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}
#endif //TEST