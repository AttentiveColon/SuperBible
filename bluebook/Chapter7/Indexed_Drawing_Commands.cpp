#include "Defines.h"
#ifdef INDEXED_DRAWING_COMMANDS
#include "System.h"
#include "Model.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

layout (location = 5)
uniform mat4 u_view_proj;

void main() 
{
	gl_Position = u_view_proj * vec4(position, 1.0);
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

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLfloat vertex_positions[] = {
	-0.25f, -0.25f, -0.25f,
	-0.25f, 0.25f, -0.25f,
	0.25f, -0.25f, -0.25f,
	0.25f, 0.25f, -0.25f,
	0.25f, -0.25f, 0.25f,
	0.25f, 0.25f, 0.25f,
	-0.25f, -0.25f, 0.25f,
	-0.25f, 0.25f, 0.25f,
};

static const GLushort vertex_indices[] = {
	0, 1, 2,
	2, 1, 3,
	2, 3, 4,
	4, 3, 5,
	4, 5, 6,
	6, 5, 7,
	6, 7, 0,
	0, 7, 1,
	6, 0, 2,
	2, 4, 6,
	7, 5, 3,
	7, 3, 1
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ibo;

	SB::Camera m_camera;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);
		m_camera = SB::Camera(std::string("Camera"), glm::vec3(-1.0f, 1.0f, -2.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		glGenBuffers(1, &m_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_positions), vertex_positions, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);

		glGenBuffers(1, &m_ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertex_indices), vertex_indices, GL_STATIC_DRAW);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();		
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);

		glUseProgram(m_program);
		glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(m_camera.m_viewproj));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
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
#endif //INDEXED_DRAWING_COMMANDS
