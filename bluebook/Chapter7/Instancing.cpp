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
uniform mat4 u_model;
layout (location = 4)
uniform mat3 u_normal_matrix;
layout (location = 5)
uniform vec3 u_view_pos;

layout (binding = 0, std140)
uniform DefaultUniform
{
	mat4 u_view;
	mat4 u_projection;
	vec2 u_resolution;
	float u_time;
};

layout (binding = 1, std140)
uniform LightUniform
{
	vec3 u_light_pos;
	float u_ambient_strength;
	vec3 u_light_color;
	float u_specular_strength;
};

out vec3 vs_normal;
out vec2 vs_uv;
out vec4 vs_frag_pos;

void main() 
{
	gl_Position = u_projection * u_view * u_model * vec4(position, 1.0);
	vs_uv = uv;
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

layout (binding = 1, std140)
uniform LightUniform
{
	vec3 u_light_pos;
	float u_ambient_strength;
	vec3 u_light_color;
	float u_specular_strength;
};

layout (location = 5) uniform vec3 u_view_pos;
layout (location = 7) uniform vec4 u_base_color_factor;
layout (location = 8) uniform float u_alpha_cutoff;

in vec3 vs_normal;
in vec2 vs_uv;
in vec4 vs_frag_pos;

layout (binding = 0)
uniform sampler2D u_texture;

layout (binding = 2)
uniform sampler2D u_normal_texture;

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

struct DefaultUniformBlock {		//std140
	glm::mat4 u_view;				//offset 0
	glm::mat4 u_proj;				//offset 16
	glm::vec2 u_resolution;			//offset 32
	GLfloat u_time;					//offset 34
};

static const GLfloat mesh0_primitive0_vertex[] = {
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

static const GLint mesh0_primitive0indices[] = {
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
	SB::Model m_model;

	SB::Camera m_camera;

	GLuint m_ubo;
	GLbyte* m_ubo_data;

	bool m_input_mode_active = true;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		input.SetRawMouseMode(window.GetHandle(), true);


		m_program = LoadShaders(shader_text);
		m_model = SB::Model("./resources/grass.glb");
		//SB::ModelDump model = SB::ModelDump("./resources/grass.glb");
		m_model.m_scale = glm::vec3(0.2f);
		m_camera = SB::Camera(std::string("Camera"), glm::vec3(-5.0f, 2.0f, 0.0f), m_model.m_position, SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);


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
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(DefaultUniformBlock), m_ubo_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);

		glUseProgram(m_program);
		glUniformMatrix3fv(5, 1, GL_FALSE, glm::value_ptr(m_camera.Eye()));
		m_model.OnDraw();
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