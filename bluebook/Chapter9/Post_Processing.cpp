#include "Defines.h"
#ifdef POST_PROCESSING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include "PostProcess.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 12)
uniform mat4 mvp;
layout (location = 13)
uniform vec3 model_pos;

out VS_OUT
{	
	vec3 normal;
	vec2 uv;
} vs_out;

void main()
{
	gl_Position = mvp * (vec4(position, 1.0) + vec4(model_pos, 1.0));
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
	color = vec4(fs_in.uv.x, fs_in.uv.y, 0.2, 1.0);
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
	PostProcess m_post_process_crt;
	PostProcess m_post_process_vhs;

	SB::Camera m_camera;
	bool m_input_mode = false;

	bool m_wireframe = false;
	ObjMesh m_cube;
	glm::vec3 m_cube_pos = glm::vec3(0.0f, 0.0f, 1.0f);

	bool m_recompile = false;

	glm::ivec2 m_resolution;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glFrontFace(GL_CCW);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);


		m_program = LoadShaders(shader_text);

		m_post_process_crt.Init("./shaders/post_process_crt.frag", window.GetWindowDimensions().width, window.GetWindowDimensions().height);
		m_post_process_vhs.Init("./shaders/post_process_vhs.frag", window.GetWindowDimensions().width, window.GetWindowDimensions().height);

		m_cube.Load_OBJ("./resources/basic_scene.obj");

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);

		m_resolution = glm::ivec2(window.GetWindowDimensions().width, window.GetWindowDimensions().height);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (m_recompile) {
			m_post_process_crt.Init("./shaders/post_process_crt.frag", window.GetWindowDimensions().width, window.GetWindowDimensions().height);
			m_post_process_vhs.Init("./shaders/post_process_vhs.frag", window.GetWindowDimensions().width, window.GetWindowDimensions().height);
			m_recompile = false;
		}

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

		float time = (float)m_time;
		glm::ivec2 resolution = m_resolution;

		m_post_process_crt.StartFrame(m_clear_color);
		RenderScene();
		m_post_process_crt.EndFrame(m_clear_color);


		m_post_process_crt.SetUniform("u_time", &time);
		m_post_process_crt.SetUniform("u_resolution", &resolution);

		m_post_process_vhs.StartFrame(m_clear_color);
		m_post_process_crt.PresentFrame();
		m_post_process_vhs.EndFrame(m_clear_color);

		m_post_process_vhs.SetUniform("u_time", &time);
		m_post_process_vhs.SetUniform("u_resolution", &resolution);

		m_post_process_vhs.PresentFrame();

		glUseProgram(0);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
		ImGui::DragFloat3("Cube Position", glm::value_ptr(m_cube_pos), 0.1f, -5.0f, 5.0f);
		if (ImGui::Button("Recompile")) {
			m_recompile = true;
		}
		ImGui::End();
	}

	void RenderScene() {
		if (m_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glUseProgram(m_program);
		glUniformMatrix4fv(12, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glUniform3fv(13, 1, glm::value_ptr(m_cube_pos));
		m_cube.OnDraw();
	}
};

SystemConf config = {
		1080,					//width
		720,					//height
		300,					//Position x
		200,					//Position y
		"Application",			//window title
		false,					//windowed fullscreen
		false,					//vsync
		30,					//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //POST_PROCESSING
