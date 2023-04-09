#include "Defines.h"
#ifdef INSTANCING
#include "System.h"
#include "Model.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) 
in vec3 position;
layout (location = 1) 
in vec3 normal;
layout (location = 2) 
in vec2 uv;

layout (location = 3)
uniform mat4 u_rotation_scale;

layout (binding = 0, std140)
uniform DefaultUniform
{
	mat4 u_view;
	mat4 u_projection;
	vec2 u_resolution;
	float u_time;
};

out vec3 vs_normal;
out vec2 vs_uv;

void main() 
{
	int x = gl_InstanceID & 0xFF;
	int z = (gl_InstanceID & 0xFF00) >> 8;
	vec4 instance_pos = vec4(float(x), 0.0, float(z), 1.0);

	mat4 translation = mat4(1.0);
	translation[3] = vec4(instance_pos);

	gl_Position = u_projection * u_view * translation * u_rotation_scale * vec4(position, 1.0);
	vs_uv = uv;
	vs_normal = normal;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

layout (binding = 0, std140)
uniform DefaultUniform
{
	mat4 u_view;
	mat4 u_projection;
	vec2 u_resolution;
	float u_time;
};

in vec3 vs_normal;
in vec2 vs_uv;

out vec4 color;

void main() 
{
	//color = vec4(vs_uv.x, vs_uv.y, 0.0, 1.0);
	color = vec4(0.2, 0.8 * (1.0 - vs_uv.x), 0.1, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct DefaultUniformBlock {		//std140
	glm::mat4 u_view;				//offset 0
	glm::mat4 u_proj;				//offset 16
	glm::vec2 u_resolution;			//offset 32
	GLfloat u_time;					//offset 34
};

static const GLfloat grass_verticies[] = {
		0.000000, -0.000000, 1.000000, 0.000000, 1.000000, -0.000000, 1.000000, 1.000000,
		0.000000, -0.000000, -1.000000, 0.000000, 1.000000, -0.000000, 1.000000, 0.000000,
		-0.177790, -0.000000, 1.000000, 0.000000, 1.000000, -0.000000, 0.911105, 1.000000,
		-0.224866, -0.000000, -1.000000, 0.000000, 1.000000, -0.000000, 0.887567, 0.000000,
		-1.932533, -0.000000, 0.616982, 0.000000, 1.000000, -0.000000, 0.033734, 0.808491,
		-0.775949, -0.000000, 0.371525, 0.000000, 1.000000, -0.000000, 0.612025, 0.685763,
		-1.433089, -0.000000, 0.251103, 0.000000, 1.000000, -0.000000, 0.283455, 0.625551,
		-0.244804, -0.000000, -0.144497, 0.000000, 1.000000, -0.000000, 0.877598, 0.427752,
		-1.807969, -0.000000, -0.446449, 0.000000, 1.000000, -0.000000, 0.096016, 0.276776
};

static const GLuint grass_indicies[] = {
		1, 3, 8,
		7, 6, 5,
		1, 8, 7,
		2, 0, 1,
		5, 4, 2,
		2, 1, 7,
		7, 5, 2
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_grass_vao;

	SB::Camera m_camera;

	GLuint m_ubo;
	GLbyte* m_ubo_data;

	bool m_input_mode_active = true;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	//TODO: create trs matrix and pass to shader to correct model orientation
	// then move onto instancing

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		input.SetRawMouseMode(window.GetHandle(), true);


		m_program = LoadShaders(shader_text);
		//SB::ModelDump model = SB::ModelDump("./resources/grass.glb");
		m_camera = SB::Camera(std::string("Camera"), glm::vec3(-5.0f, 2.0f, 0.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);

		//Create VAO, Describe and bind all related vertex and index buffers
		//

		//Create vertex array object and vertex/index buffers
		glCreateVertexArrays(1, &m_grass_vao);
		glBindVertexArray(m_grass_vao);
		GLuint vertex_buffer, index_buffer;
		glCreateBuffers(1, &vertex_buffer);
		glCreateBuffers(1, &index_buffer);
		glNamedBufferStorage(vertex_buffer, sizeof(grass_verticies), grass_verticies, 0);
		glNamedBufferStorage(index_buffer, sizeof(grass_indicies), grass_indicies, 0);

		//Describe layout
		//Positions
		glVertexArrayAttribBinding(m_grass_vao, 0, 0);
		glVertexArrayAttribFormat(m_grass_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glEnableVertexArrayAttrib(m_grass_vao, 0);
		//Normals
		glVertexArrayAttribBinding(m_grass_vao, 1, 0);
		glVertexArrayAttribFormat(m_grass_vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
		glEnableVertexArrayAttrib(m_grass_vao, 1);
		//UVs
		glVertexArrayAttribBinding(m_grass_vao, 2, 0);
		glVertexArrayAttribFormat(m_grass_vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
		glEnableVertexArrayAttrib(m_grass_vao, 2);

		//Pass buffers to VAO
		glVertexArrayVertexBuffer(m_grass_vao, 0, vertex_buffer, 0, sizeof(float) * 8);
		glVertexArrayElementBuffer(m_grass_vao, index_buffer);
		glBindVertexArray(0);

		glCreateBuffers(1, &m_ubo);
		m_ubo_data = new GLbyte[sizeof(DefaultUniformBlock)];
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

		DefaultUniformBlock ubo;
		ubo.u_view = m_camera.m_view;
		ubo.u_proj = m_camera.m_proj;
		ubo.u_resolution = glm::vec2((float)window.GetWindowDimensions().width, (float)window.GetWindowDimensions().height);
		ubo.u_time = (float)m_time;
		memcpy(m_ubo_data, &ubo, sizeof(DefaultUniformBlock));
	}
	void OnDraw() {
		static const glm::mat4 rotation_scale = glm::rotate(glm::radians(-90.0f), vec3(0.0f, 0.0f, 1.0f)) * glm::scale(vec3(0.2f, 0.2f, 0.2f));
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		

		glUseProgram(m_program);

		//bind uniform buffer
		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(DefaultUniformBlock), m_ubo_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);
		
		//Draw using the gl_InstanceID to position instances of model
		//
		glBindVertexArray(m_grass_vao);
		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(rotation_scale));
		//glDrawElements(GL_TRIANGLES, 21, GL_UNSIGNED_INT, (void*)0);
		glDrawElementsInstanced(GL_TRIANGLES, 21, GL_UNSIGNED_INT, (void*)0, 0xFFFF);
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