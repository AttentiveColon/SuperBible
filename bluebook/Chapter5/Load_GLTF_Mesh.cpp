#include "Defines.h"
#ifdef LOAD_GLTF_MESH
#include "System.h"
#include "Model.h"
#include <string>
#include <vector>

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) uniform mat4 u_viewproj;
layout (location = 3) uniform mat4 u_model;
layout (location = 4) uniform mat3 u_normal_matrix;
layout (location = 5) uniform float u_time;
layout (location = 6) uniform vec2 u_resolution;

layout (location = 8) uniform float u_blend;

out vec4 vs_normal;
out vec2 vs_uv;

void main() 
{
	vec4 new_pos = u_viewproj * u_model * vec4(position.x, position.y, position.z, 1.0);
	gl_Position = new_pos;
	vs_normal = u_model * vec4(normal, 1.0);
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

layout (location = 7) uniform vec4 u_base_color_factor;

in vec4 vs_normal;
in vec2 vs_uv;

layout (binding = 0) uniform sampler2D u_texture;
layout (binding = 1) uniform sampler2D u_metallic;
layout (binding = 2) uniform sampler2D u_normal_map;
layout (binding = 3) uniform sampler2D u_occlusion_map;
layout (binding = 4) uniform sampler2D u_emissive_map;

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

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	SB::Model m_model;
	glm::mat4 m_viewproj;
	glm::vec3 m_cam_pos;
	float m_cam_rotation;

	SB::Camera m_camera;
	

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		m_cam_pos(glm::vec3(0.0f, 10.0, 12.0)),
		m_cam_rotation(0.0f)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		//audio.PlayOneShot("./resources/startup.mp3");
		m_program = LoadShaders(shader_text);
		m_model = SB::Model("./resources/ABeautifulGame.glb");

		if (m_model.m_camera.m_cameras.size()) {
			m_camera = m_model.m_camera.GetCamera(0);
		}
		else {
			m_camera = SB::Camera("Camera", glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.40, 0.1, 100.0);
		}

		m_viewproj = m_camera.ViewProj();
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

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

		
		m_viewproj = m_camera.ViewProj();
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_viewproj));
		glUniform1f(5, (float)m_time);
		glUniform2i(6, 1600, 900);
		m_model.OnDraw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", (int)m_fps);
		ImGui::Text("Time: %f", (double)m_time);
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
		144,					//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //LOAD_GLTF_MESH
