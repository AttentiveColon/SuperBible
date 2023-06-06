#include "Defines.h"
#ifdef SKYBOX
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* skybox_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

out VS_OUT
{
    vec3    tc;
} vs_out;

layout (location = 1)
uniform mat4 u_view;
layout (location = 2)
uniform mat4 u_proj;

void main(void)
{
	vec4 pos = u_proj * u_view * vec4(position, 1.0);
	gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
	vs_out.tc = vec3(position.x, position.y, -position.z);
}

)";

static const GLchar* skybox_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform samplerCube tex_cubemap;

in VS_OUT
{
    vec3    tc;
} fs_in;

layout (location = 0) out vec4 color;

void main(void)
{
    color = texture(tex_cubemap, fs_in.tc);
}
)";

static ShaderText skybox_shader_text[] = {
	{GL_VERTEX_SHADER, skybox_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, skybox_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_skybox_program;

	ObjMesh m_cube;
	GLuint m_cube_map;


	SB::Camera m_camera;
	bool m_input_mode = false;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_skybox_program = LoadShaders(skybox_shader_text);

		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 2000.0);

		m_cube.Load_OBJ("./resources/cube.obj");
		m_cube_map = Load_KTX("./resources/mountaincube.ktx");
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_cube_map);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (m_input_mode) {
			m_camera.OnUpdate(input, 3.0f, 0.2f, dt);
		}

		//Implement Camera Movement Functions
		if (input.Pressed(GLFW_KEY_LEFT_CONTROL)) {
			m_input_mode = !m_input_mode;
			input.SetRawMouseMode(window.GetHandle(), m_input_mode);
		}
	}
	void OnDraw() {
		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glUseProgram(m_skybox_program);
		glBindTextureUnit(0, m_cube_map);
		//Cast to mat3 then back to mat4 so last row is zero, thus having no effect on translations, then you will always be at the center of the skybox
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(m_camera.m_view)))); 
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		m_cube.OnDraw();
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
#endif //SKYBOX