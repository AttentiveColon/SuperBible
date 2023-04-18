#include "Defines.h"
#ifdef POINTS_TO_CUBES
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 450 core

void main()
{
	gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}
)";

static const GLchar* geometry_shader_source = R"(
#version 450 core

layout (points) in;
layout (triangle_strip) out;
layout (max_vertices = 18) out;

layout (location = 12)
uniform mat4 mvp;

out GS_OUT
{
	vec2 uv;
} gs_out;

const vec4 offsets[] = {
	vec4(-1.0, -1.0, -1.0, 1.0),
	vec4(-1.0, 1.0, -1.0, 1.0),
	vec4(1.0, -1.0, -1.0, 1.0), 
	vec4(1.0, 1.0, -1.0, 1.0),
	vec4(1.0, -1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(-1.0, -1.0, 1.0, 1.0),
	vec4(-1.0, 1.0, 1.0, 1.0),
	vec4(-1.0, -1.0, -1.0, 1.0),
	vec4(-1.0, 1.0, -1.0, 1.0),
	vec4(-1.0, 1.0, -1.0, 1.0),
	vec4(-1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, -1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(-1.0, -1.0, -1.0, 1.0),
	vec4(-1.0, -1.0, 1.0, 1.0),
	vec4(1.0, -1.0, -1.0, 1.0),
	vec4(1.0, -1.0, 1.0, 1.0)
};

void main()
{
	vec4 position = gl_in[0].gl_Position;
	for (int i = 0; i < 10; ++i)
	{
		gl_Position = mvp * (position + offsets[i]);
		gs_out.uv = vec2(1.0, 1.0);
		EmitVertex();
	}
	EndPrimitive();
	for (int i = 10; i < 14; ++i)
	{
		gl_Position = mvp * (position + offsets[i]);
		gs_out.uv = vec2(1.0, 1.0);
		EmitVertex();
	}
	EndPrimitive();
	for (int i = 14; i < 18; ++i)
	{
		gl_Position = mvp * (position + offsets[i]);
		gs_out.uv = vec2(1.0, 1.0);
		EmitVertex();
	}
	EndPrimitive();
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

in GS_OUT
{
	vec2 uv;
} fs_in;

void main()
{
	color = vec4(gl_FragCoord.x, gl_FragCoord.y, 1.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_GEOMETRY_SHADER, geometry_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};



struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_vao;

	SB::Camera m_camera;
	bool m_input_mode = false;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		m_program = LoadShaders(shader_text);

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 1.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);
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
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);
		glPointSize(15.0f);

		glBindVertexArray(m_vao);
		glUseProgram(m_program);
		glUniformMatrix4fv(12, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_POINTS, 0, 1);
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
#endif //POINTS_TO_CUBES
