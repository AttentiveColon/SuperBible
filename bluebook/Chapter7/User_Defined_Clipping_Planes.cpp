#include "Defines.h"
#ifdef USER_DEFINED_CLIPPING_PLANES
#include "System.h"
#include "Model.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

layout (location = 5)
uniform mat4 mvp;

out vec4 vs_frag_pos;

void main() 
{
	gl_Position = mvp * vec4(position, 1.0);
	vs_frag_pos = mvp * vec4(position, 1.0);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in vec4 vs_frag_pos;

out vec4 color;

void main() 
{
	
	color = vs_frag_pos;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLfloat object[] = {
	-0.5, 0.5, -0.5,	//top front left		//0
	0.5, 0.5, -0.5,		//top front right		//1
	-0.5, -0.5, -0.5,	//bottom front left		//2
	0.5, -0.5, -0.5,	//bottom front right	//3
	-0.5, 0.5, 0.5,		//top back left			//4
	0.5, 0.5, 0.5,		//top back right		//5
	-0.5, -0.5, 0.5,	//bottom back left		//6
	0.5, -0.5, 0.5,		//bottom back right		//7
};

static const GLuint object_elements[] = {
	0, 1, 2,
	2, 1, 3,
	0, 6, 4,
	0, 6, 2,
	1, 7, 5,
	1, 7, 3,
	4, 5, 6,
	6, 5, 7,
	2, 7, 6,
	2, 7, 3,
	0, 5, 4,
	0, 5, 1,
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_vao;

	SB::Camera m_camera;
	bool m_input_mode = true;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		m_program = LoadShaders(shader_text);
		glCreateVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);
		GLuint vbo, ebo;
		glCreateBuffers(1, &vbo);
		glCreateBuffers(1, &ebo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(object), object, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(object_elements), object_elements, GL_STATIC_DRAW);

		glBindVertexArray(0);


		m_camera = SB::Camera("camera", glm::vec3(0.0f, 2.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (input.Pressed(GLFW_KEY_LEFT_CONTROL)) {
			input.SetRawMouseMode(window.GetHandle(), m_input_mode);
			m_input_mode = !m_input_mode;
		}

		m_camera.OnUpdate(input, 3.0f, 0.02f, dt);
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj() * glm::scale(glm::vec3(1.0, 1.0, 10.0))));
		glDrawElements(GL_TRIANGLES, sizeof(object_elements) / sizeof(object_elements[0]), GL_UNSIGNED_INT, 0);
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
#endif //USER_DEFINED_CLIPPING_PLANES
