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

layout (location = 3) uniform mat4 u_model;
layout (location = 4) uniform mat4 u_viewproj;
layout (location = 5) uniform float u_time;
layout (location = 6) uniform vec2 u_resolution;

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

in vec4 vs_normal;
in vec2 vs_uv;

out vec4 color;

void main() 
{
	color = vec4(vs_uv.x, vs_uv.y, 0.0, 1.0);
	//color = vec4(0.5 * (vs_normal.y * vs_normal.x + 0.1) + 0.1, 0.5 * (vs_normal.y * (vs_normal.z + 0.2)) + 0.1, 0.5 * (vs_normal.y * (vs_normal.x + 0.3)) + 0.1, 1.0);
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
	SB2::Model m_model;
	glm::mat4 m_viewproj;
	glm::vec3 m_cam_pos;
	float m_cam_rotation;
	

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		m_cam_pos(glm::vec3(0.0f, 0.3, 0.5)),
		m_cam_rotation(0.0f)
	{}
	//TODO: Load every mesh into GL buffers once, have nodes contain only the buffer GLuint to mesh
	// so you dont duplicate meshes across multiple buffers
	//TODO: Figure out loading children of pawn meshes

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		audio.PlayOneShot("./resources/startup.mp3");
		m_program = LoadShaders(shader_text);
		m_model = SB2::Model("./resources/ABeautifulGame.glb");

		glm::mat4 lookat = glm::lookAt(m_cam_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0, 0.0));
		glm::mat4 perspective = glm::perspective(90.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
		m_viewproj = perspective * lookat;
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();


		glm::mat4 lookat = glm::lookAt(m_cam_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0, 0.0));
		glm::mat4 perspective = glm::perspective(90.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
		m_viewproj = perspective * lookat;
		
		m_cam_rotation += 0.1f;
		m_viewproj *= glm::rotate(glm::mat4(1.0f), glm::radians(m_cam_rotation), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program);
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(m_viewproj));
		glUniform1f(5, (float)m_time);
		glUniform2i(6, 1600, 900);
		m_model.OnDraw();
		//m_model.m_scenes[m_model.m_current_scene].m_nodes[0].OnDraw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat3("CamPos", glm::value_ptr(m_cam_pos), -10.0f, 10.0f);
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
#endif //TEST
