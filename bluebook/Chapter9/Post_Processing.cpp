#include "Defines.h"
#ifdef POST_PROCESSING
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

static const GLchar* fragment_shader_source2 = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D u_texture;

out vec4 color;

in VS_OUT
{
	vec2 uv;
} fs_in;

void main()
{
	
	color = texture(u_texture, fs_in.uv * float(gl_FragCoord.x / 1600.0));
}
)";


struct PostProcess {
	GLuint m_program;
	GLuint m_fbo;
	GLuint m_texture;
	GLuint m_depth;

	PostProcess() :m_program(0), m_fbo(0), m_texture(0), m_depth(0) {}
	void Init(const GLchar* fragment_shader, int window_width, int window_height);
	void StartFrame(float clear_color[4]);
	void EndFrame(float clear_color[4]);
	void PresentFrame();
};

void PostProcess::Init(const GLchar* fragment_shader, int window_width, int window_height) {
	static const GLchar* pp_vertex_shader_source = R"(
#version 450 core
out VS_OUT
{	
	vec2 uv;
} vs_out;

void main()
{
	const vec2 vertices[] = 
	{
		vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0),
		vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(1.0, -1.0)
	};
	const vec2 uv[] =
	{
		vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0),
		vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 0.0)
	};
	gl_Position = vec4(vertices[gl_VertexID], 0.5, 1.0);
	vs_out.uv = uv[gl_VertexID];
}
)";
	static ShaderText pp_shader_text[] = {
		{GL_VERTEX_SHADER, pp_vertex_shader_source, NULL},
		{GL_FRAGMENT_SHADER, fragment_shader, NULL},
		{GL_NONE, NULL, NULL}
	};

	m_program = LoadShaders(pp_shader_text);

	static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };

	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, window_width, window_height);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture(GL_FRAMEBUFFER, draw_buffers[0], m_texture, 0);

	glGenTextures(1, &m_depth);
	glBindTexture(GL_TEXTURE_2D, m_depth);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, window_width, window_height);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depth, 0);

	glDrawBuffers(1, draw_buffers);

	//Unbind resources
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void PostProcess::StartFrame(float clear_color[4]) {
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glClearBufferfv(GL_COLOR, 0, clear_color);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);
}

void PostProcess::EndFrame(float clear_color[4]) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearBufferfv(GL_COLOR, 0, clear_color);
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_STENCIL_BUFFER_BIT);
}

void PostProcess::PresentFrame() {
	glUseProgram(m_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTextureUnit(0, m_texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}


struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	PostProcess m_post_process;

	SB::Camera m_camera;
	bool m_input_mode = false;

	bool m_wireframe = false;
	ObjMesh m_cube;
	glm::vec3 m_cube_pos = glm::vec3(0.0f, 0.0f, 1.0f);


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

		m_post_process.Init(fragment_shader_source2, window.GetWindowDimensions().width, window.GetWindowDimensions().height);

		m_cube.Load_OBJ("./resources/cube.obj");

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

		m_post_process.StartFrame(m_clear_color);

		RenderScene();

		m_post_process.EndFrame(m_clear_color);
		m_post_process.PresentFrame();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
		ImGui::DragFloat3("Cube Position", glm::value_ptr(m_cube_pos), 0.1f, -5.0f, 5.0f);
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
#endif //POST_PROCESSING
