#include "Defines.h"
#ifdef VERTEX_ATTRIB_DIVISOR
#include "System.h"
#include "Model.h"
#include "Texture.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec4 position;
layout (location = 1)
in vec4 instance_color;
layout (location = 2)
in vec4 instance_position;

out Fragment
{
	vec4 color;
} fragment;

layout (location = 3)
uniform mat4 mvp;

void main() 
{
	gl_Position = mvp * (position + instance_position);
	fragment.color = instance_color;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in Fragment
{
	vec4 color;
} fragment;

out vec4 color;

void main() 
{
	color = fragment.color;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLfloat square_vertices[] = {
	-1.0f, -1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f,
	-1.0f, 1.0f, 0.0f, 1.0f
};

static const GLfloat instance_colors[] = {
	1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f
};

static const GLfloat instance_positions[] = {
	-2.0f, -2.0f, 0.0f, 0.0f, 
	2.0f, -2.0f, 0.0f, 0.0f,
	2.0f, 2.0f, 0.0f, 0.0f,
	-2.0f, 2.0f, 0.0f, 0.0f
};



struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_square_vao;
	GLuint m_square_vbo;

	SB::Camera m_camera;

	bool m_input_mode_active = true;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		input.SetRawMouseMode(window.GetHandle(), true);


		m_program = LoadShaders(shader_text);
		//SB::ModelDump model = SB::ModelDump("./resources/grass.glb");
		m_camera = SB::Camera(std::string("Camera"), glm::vec3(0.0f, 2.0f, -5.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);


		//Create buffers and bind VAO
		glGenVertexArrays(1, &m_square_vao);
		glGenBuffers(1, &m_square_vbo);
		glBindVertexArray(m_square_vao);


		//Create buffer large enough to store all three arrays
		GLuint offset = 0;
		glBindBuffer(GL_ARRAY_BUFFER, m_square_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices) + sizeof(instance_colors) + sizeof(instance_positions), NULL, GL_STATIC_DRAW);

		//Place data into buffers at appropriate offsets
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(square_vertices), square_vertices);
		offset += sizeof(square_vertices);
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(instance_colors), instance_colors);
		offset += sizeof(instance_colors);
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(instance_positions), instance_positions);
		//offset += sizeof(instance_positions);

		//Describe the locations of each array in the buffer
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeof(square_vertices));
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(square_vertices) + sizeof(instance_colors)));

		//Enable those attributes
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		//Turn attribute 1 and 2 into per-instance values
		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (m_input_mode_active) {
			m_camera.OnUpdate(input, 3.0f, 0.2f, dt);
		}

		//Implement Camera Movement Functions
		if (input.Pressed(GLFW_KEY_LEFT_CONTROL)) {
			m_input_mode_active = !m_input_mode_active;
			input.SetRawMouseMode(window.GetHandle(), m_input_mode_active);
		}

		
	}
	void OnDraw() {
		
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);


		glUseProgram(m_program);
		glBindVertexArray(m_square_vao);
		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(m_camera.m_viewproj));
		glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 4);

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
#endif //INSTANCING