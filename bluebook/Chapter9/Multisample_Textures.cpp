#include "Defines.h"
#ifdef MULTISAMPLE_TEXTURES
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
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* default_vertex_shader_source = R"(
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

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2DMS u_texture;

out vec4 color;

in VS_OUT
{
	vec2 uv;
} fs_in;

void main()
{
	ivec2 uv = ivec2(gl_FragCoord.xy);
	vec4 max_result = vec4(0.0);
	vec4 min_result = vec4(1.0);

	for (int i = 0; i < 8; ++i)
	{
		max_result = max(max_result, texelFetch(u_texture, uv, i));
		min_result = min(min_result, texelFetch(u_texture, uv, i));
	}
	color = max_result - min_result;
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

	GLuint m_program, m_program2;
	SB::Camera m_camera;
	bool m_input_mode = false;

	bool m_wireframe = false;
	ObjMesh m_cube;
	glm::vec3 m_cube_pos = glm::vec3(0.0f, 0.0f, 1.0f);

	GLuint m_fbo, m_color_ms_tex, m_depth_ms_tex;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);
		m_program2 = LoadShaders(default_shader_text);

		m_cube.Load_OBJ("./resources/monkey.obj");

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);

		//Create multisampled color texture
		glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_color_ms_tex);
		glTextureStorage2DMultisample(m_color_ms_tex, 8, GL_RGBA8, window.GetWindowDimensions().width, window.GetWindowDimensions().height, GL_TRUE);
		//Create multisampled depth texture
		glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_depth_ms_tex);
		glTextureStorage2DMultisample(m_depth_ms_tex, 8, GL_DEPTH_COMPONENT32, window.GetWindowDimensions().width, window.GetWindowDimensions().height, GL_TRUE);

		//Create FBO
		glGenFramebuffers(1, &m_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_color_ms_tex, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depth_ms_tex, 0);

		assert(glCheckNamedFramebufferStatus(m_fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
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


		//Bind framebuffer and render out to our multisampled textures
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		static const GLuint draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, draw_buffers);

		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		RenderScene();
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);
		glDisable(GL_DEPTH_TEST);
		glClearBufferfv(GL_COLOR, 0, m_clear_color);


		//Render our multisampled texture
		//This example renders the color different between the max and min values within the multisampled pixel
		glUseProgram(m_program2);
		glActiveTexture(GL_TEXTURE0);
		glBindTextureUnit(0, m_color_ms_tex);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindTextureUnit(0, 0);
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
		glUniformMatrix4fv(10, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glUniform4fv(11, 1, glm::value_ptr(m_cube_pos));
		m_cube.OnDraw();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
#endif //MULTISAMPLE_TEXTURES
