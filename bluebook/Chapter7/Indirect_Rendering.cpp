#include "Defines.h"
#ifdef INDIRECT_RENDERING
#include "System.h"
#include "Model.h"
#include "Texture.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 3)
uniform mat4 mvp;

out vec2 vs_uv;

void main() 
{
	gl_Position = mvp * vec4(position, 1.0);
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in vec2 vs_uv;

out vec4 color;

void main() 
{
	color = vec4(vs_uv.x, vs_uv.y, 0.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLfloat square_vertices[] = {
		-0.500000, 0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 1.000000,
		0.500000, 0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 1.000000,
		-0.500000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 0.000000,
		0.500000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 0.000000
};

static const GLuint square_indices[] = {
		0, 1, 3,
		0, 3, 2
};

static const GLfloat triangle_vertices[] = {
		-0.500000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 1.000000,
		0.500000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 0.000000,
		0.000000, 0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 1.000000
};

static const GLuint triangle_indices[] = {
		1, 2, 0
};

static const GLfloat diamond_vertices[] = {
		-0.250000, 0.000000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 1.000000,
		0.000000, 0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 1.000000,
		0.000000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 0.000000,
		0.250000, 0.000000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 0.000000
};

static const GLuint diamond_indices[] = {
		0, 1, 3,
		0, 3, 2
};

static const GLfloat trapezoid_vertices[] = {
		-0.500000, -0.500000, -0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 1.000000,
		0.000000, -0.500000, -0.000000, 0.000000, 0.000000, 1.000000, 1.000000, 1.000000,
		0.000000, 0.500000, -0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000,
		0.500000, 0.500000, -0.000000, 0.000000, 0.000000, 1.000000, 1.000000, 0.000000
};

static const GLuint trapezoid_indices[] = {
		0, 1, 3,
		0, 3, 2
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
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

	bool m_input_mode_active = true;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		//input.SetRawMouseMode(window.GetHandle(), true);


		m_program = LoadShaders(shader_text);
		//SB::ModelDump model = SB::ModelDump("./resources/indirect_render_model.glb");
		m_camera = SB::Camera(std::string("Camera"), glm::vec3(0.0f, 2.0f, 5.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);


		//Create buffers and bind VAO
		glCreateVertexArrays(1, &m_vao);
		glCreateBuffers(1, &m_vbo);


		//Create buffer large enough to store all vertex data
		size_t buffer_size = sizeof(square_vertices) + sizeof(triangle_vertices) + sizeof(diamond_vertices) + sizeof(trapezoid_vertices);
		glNamedBufferData(m_vbo, buffer_size, NULL, GL_STATIC_DRAW);

		//Place data into buffers at appropriate offsets
		GLuint offset = 0;
		glNamedBufferSubData(m_vbo, offset, sizeof(square_vertices), square_vertices);
		offset += sizeof(square_vertices);
		glNamedBufferSubData(m_vbo, offset, sizeof(triangle_vertices), triangle_vertices);
		offset += sizeof(triangle_vertices);
		glNamedBufferSubData(m_vbo, offset, sizeof(diamond_vertices), diamond_vertices);
		offset += sizeof(diamond_vertices);
		glNamedBufferSubData(m_vbo, offset, sizeof(trapezoid_vertices), trapezoid_vertices);

		//Describe the locations of each array in the buffer
		glVertexArrayAttribBinding(m_vao, 0, 0);
		glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(m_vao, 1, 0);
		glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
		glVertexArrayAttribBinding(m_vao, 2, 0);
		glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
		
		//Enable those attributes
		glEnableVertexArrayAttrib(m_vao, 0);
		glEnableVertexArrayAttrib(m_vao, 1);
		glEnableVertexArrayAttrib(m_vao, 2);

		//Bind buffer to VAO
		glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(float) * 8);
		
		//Create buffer to store all indices data
		glCreateBuffers(1, &m_ibo);
		buffer_size = sizeof(square_indices) + sizeof(triangle_indices) + sizeof(diamond_indices) + sizeof(trapezoid_indices);
		glNamedBufferData(m_ibo, buffer_size, NULL, GL_STATIC_DRAW);

		//Place indice data
		offset = 0;
		glNamedBufferSubData(m_ibo, offset, sizeof(square_indices), square_indices);
		offset += sizeof(square_indices);
		glNamedBufferSubData(m_ibo, offset, sizeof(triangle_indices), triangle_indices);
		offset += sizeof(triangle_indices);
		glNamedBufferSubData(m_ibo, offset, sizeof(diamond_indices), diamond_indices);
		offset += sizeof(diamond_indices);
		glNamedBufferSubData(m_ibo, offset, sizeof(trapezoid_indices), trapezoid_indices);

		//Bind index buffer to VAO
		glVertexArrayElementBuffer(m_vao, m_ibo);

		//Unbind buffers
		glBindVertexArray(0);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (m_input_mode_active) {
			//m_camera.OnUpdate(input, 3.0f, 0.2f, dt);
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
		glBindVertexArray(m_vao);
		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(m_camera.m_viewproj));

		int time = (int)m_time;

		//draw square
		if (time % 4 == 0) glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * 0), 0);
		//draw triangle
		else if (time % 4 == 1) glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * 6), 4);
		//draw diamond
		else if (time % 4 == 2) glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * 9), 7);
		//draw trapezoid
		else glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * 15), 11);
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
#endif //INDIRECT_RENDERING