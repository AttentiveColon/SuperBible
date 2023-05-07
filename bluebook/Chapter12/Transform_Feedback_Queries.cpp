#include "Defines.h"
#ifdef TRANSFORM_FEEDBACK_QUERIES
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* default_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 4)
uniform mat4 u_viewProj;

out VS_OUT
{
	vec3 normal;
	vec2 uv;
} vs_out;

void main(void)
{
    gl_Position = u_viewProj * vec4(position, 1.0);
	vs_out.normal = normal;
	vs_out.uv = uv;
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

in VS_OUT
{
	vec3 normal;
	vec2 uv;
} fs_in;

out vec4 color;

void main()
{
	color = vec4(fs_in.uv.x, fs_in.uv.y, 0.0, 1.0);
}
)";

static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program, m_occlusion_program;
	GLuint m_ubo;

	GLuint m_queries;

	ObjMesh m_cube;

	SB::Camera m_camera;
	bool m_input_mode = false;

	Random m_random;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(default_shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 2.0f, -12.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_cube.Load_OBJ("./resources/cube.obj");
		m_random.Init();

		glGenQueries(1, &m_queries);
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

		glEnable(GL_DEPTH_TEST);

		glUseProgram(m_program);
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
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
#endif //TRANSFORM_FEEDBACK_QUERIES