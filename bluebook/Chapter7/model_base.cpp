#include "Defines.h"
#ifdef MODEL_BASE
#include "System.h"
#include "Model.h"
#include <string>
#include <vector>

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

layout (binding = 0, std140)
uniform DefaultUniform
{
	mat4 u_view;
	mat4 u_projection;
	vec2 u_resolution;
	float u_time;
};

out vec4 vs_normal;
out vec2 vs_uv;

void main() 
{
	mat4 viewProj = u_projection * u_view;
	gl_Position = viewProj * u_model * vec4(position, 1.0);
	vs_normal = viewProj * vec4(normal, 1.0);
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

layout (location = 7) uniform vec4 u_base_color_factor;

in vec4 vs_normal;
in vec2 vs_uv;

uniform sampler2D u_texture;

out vec4 color;

void main() 
{
	color = texture(u_texture, vs_uv) * u_base_color_factor;
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

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	f64 m_dt;

	GLuint m_program;
	SB::Model m_model;
	glm::vec3 m_cam_pos;
	float m_cam_rotation;

	SB::Camera m_camera;

	GLuint m_ubo;
	GLbyte* m_ubo_data;


	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		m_cam_pos(glm::vec3(0.0f, 10.0, 12.0)),
		m_cam_rotation(0.0f)
	{}

	//TODO: fix Mesh::OnDraw so it doesn't need to be passed the material array
	// --- also make sure it works when passed no material
	//TODO: Figure out where information about blend state is stored in gltf structure
	

	//TODO: Look into and try to limit excessive loading on startup
	// (might have to create script to create custom model format)

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		//audio.PlayOneShot("./resources/startup.mp3");
		m_program = LoadShaders(shader_text);
		m_model = SB::Model("./resources/sponza.glb");
		//m_model = SB::Model("../gltf_examples/2.0/sponza/glTF/Sponza.gltf");

		input.SetRawMouseMode(window.GetHandle(), true);
		if (m_model.m_camera.m_cameras.size()) {
			m_camera = m_model.m_camera.GetCamera(0);
		}
		else {
			m_camera = SB::Camera("Camera", glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.40, 0.1, 100.0);
		}

		glCreateBuffers(1, &m_ubo);
		m_ubo_data = new GLbyte[sizeof(DefaultUniformBlock)];
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
		m_dt = dt;

		if (input.Pressed(GLFW_KEY_SPACE)) {
			m_camera = m_model.m_camera.GetNextCamera();
		}

		//Implement Camera Movement Functions
		if (input.Held(GLFW_KEY_LEFT_CONTROL)) {
			input.SetRawMouseMode(window.GetHandle(), false);
		}
		else {
			input.SetRawMouseMode(window.GetHandle(), true);
			m_camera.OnUpdate(input, 3.0f, 2.5f, dt);
		}


		DefaultUniformBlock ubo;
		ubo.u_view = m_camera.m_view;
		ubo.u_proj = m_camera.m_proj;
		ubo.u_resolution = glm::vec2((float)window.GetWindowDimensions().width, (float)window.GetWindowDimensions().height);
		ubo.u_time = (float)m_time;

		memcpy(m_ubo_data, &ubo, sizeof(DefaultUniformBlock));
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(DefaultUniformBlock), m_ubo_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);

		glUseProgram(m_program);
		m_model.OnDraw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", (int)m_fps);
		ImGui::Text("Time: %f", (double)m_time);
		ImGui::Text("Frame Time: %f", (double)m_dt);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::LabelText("Current Camera", "{%d}", m_model.m_current_camera);
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
		30,						//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //LOAD_GLTF_MESH
