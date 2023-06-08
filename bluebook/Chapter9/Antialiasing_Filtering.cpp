#include "Defines.h"
#ifdef ANTIALIASING_FILTERING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 10)
uniform mat4 u_viewproj;
layout (location = 11)
uniform vec4 u_model_pos;

out VS_OUT
{	
	vec3 normal;
	vec2 uv;
} vs_out;

void main()
{
	gl_Position = u_viewproj * (u_model_pos + vec4(position, 1.0));
	vs_out.normal = normal;
	vs_out.uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

in VS_OUT
{
	vec3 normal;
	vec2 uv;
} fs_in;

void main()
{
	color = vec4(fs_in.uv.x, fs_in.uv.y, 0.0, 1.0);
	//color = vec4(1.0);
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
	SB::Camera m_camera;
	bool m_input_mode = false;

	bool m_wireframe = false;
	ObjMesh m_cube;
	glm::vec4 m_cube_pos = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	bool m_line_smooth = false;
	bool m_polygon_smooth = false;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);

		m_cube.Load_OBJ("./resources/smooth_sphere.obj");

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);
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
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		//For filtered anti-aliasing we need to enable blending
		//MSAA needs to be disabled for this example to work
		//if using MSAA you enable with
		//glEnable(GL_MULTISAMPLE);

		if (m_line_smooth) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//then we enable GL_LINE_SMOOTH to apply anti-aliasing to lines
			glEnable(GL_LINE_SMOOTH);
		}
		else {
			glDisable(GL_BLEND);
			glDisable(GL_LINE_SMOOTH);
		}

		if (m_polygon_smooth) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//and enable GL_POLYGON_SMOOTH for triangles
			glEnable(GL_POLYGON_SMOOTH);
		}
		else {
			glDisable(GL_BLEND);
			glDisable(GL_POLYGON_SMOOTH);
		}


		if (m_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glUseProgram(m_program);
		glUniformMatrix4fv(10, 1, GL_FALSE, glm::value_ptr(m_camera.m_viewproj));
		glUniform4fv(11, 1, glm::value_ptr(m_cube_pos));
		m_cube.OnDraw();

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
		ImGui::DragFloat3("Cube Position", glm::value_ptr(m_cube_pos), 0.1f, -5.0f, 5.0f);
		ImGui::Checkbox("Line Smooth", &m_line_smooth);
		ImGui::Checkbox("Polygon Smooth", &m_polygon_smooth);
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
#endif //ANTIALIASING_FILTERING
