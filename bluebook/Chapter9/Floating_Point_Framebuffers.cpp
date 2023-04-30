#include "Defines.h"
#ifdef FLOATING_POINT_FRAMEBUFFERS
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

layout (binding = 0)
uniform sampler2D u_texture;

layout (location = 4)
uniform bool u_active;

in VS_OUT
{
	vec3 normal;
	vec2 uv;
} fs_in;

void main()
{
	vec4 c = texture(u_texture, fs_in.uv);

	vec4 c0 = c;
	vec4 c1 = c;
	vec4 c2 = c;

	c0.rgb = vec3(1.0) - exp(-c0.rgb * 1.5);
	c1.rgb = vec3(1.0) - exp(-c1.rgb * 2.5);
	c2.rgb = vec3(1.0) - exp(-c2.rgb * 6.5);
	
	color = (c0 + c1 + c2) / 3.0;

	if (!u_active)
	{
		color = texture(u_texture, fs_in.uv);
	}
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
uniform sampler2D u_texture;

layout (binding = 1)
uniform sampler1D u_tex_lut;

layout (location = 4)
uniform bool u_active;

layout (location = 1)
uniform float u_exposure;

out vec4 color;

in VS_OUT
{
	vec2 uv;
} fs_in;

void main()
{
	ivec2 uv = ivec2(gl_FragCoord.xy);
	color = texelFetch(u_texture, uv, 0);
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
	GLuint m_tex, m_tex_lut;
	glm::vec3 m_cube_pos = glm::vec3(0.0f, 0.0f, 1.0f);

	GLuint m_fbo, m_color_tex1, m_color_tex2, m_color_tex3, m_depth_tex;
	bool m_active = true;
	float m_exposure = 1.0f;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);
		m_program2 = LoadShaders(default_shader_text);

		m_cube.Load_OBJ("./resources/cube.obj");

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);
		m_tex = Load_KTX("./resources/fiona.ktx");

		static const GLfloat exposureLUT[20] = { 11.0f, 6.0f, 3.2f, 2.8f, 2.2f, 1.90f, 1.80f, 1.80f, 1.70f, 1.70f, 1.60f, 1.60f, 1.50f, 1.50f, 1.40f, 1.40f, 1.30f, 1.20f, 1.10f, 1.00f };

		glGenTextures(1, &m_tex_lut);
		glBindTexture(GL_TEXTURE_1D, m_tex_lut);
		glTexStorage1D(GL_TEXTURE_1D, 1, GL_R32F, 20);
		glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 20, GL_RED, GL_FLOAT, exposureLUT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

		FBOSetup(window);
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
		//glActiveTexture(GL_TEXTURE0);
		glBindTextureUnit(0, m_color_tex1);
		glBindTextureUnit(1, m_tex_lut);
		glUniform1i(4, m_active);
		glUniform1f(1, m_exposure);
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
		ImGui::Checkbox("Shader Active", &m_active);
		ImGui::DragFloat("Exposure", &m_exposure, 0.1f, 0.01f, 20.0f);
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
		glUniform1i(4, m_active);
		glBindTextureUnit(0, m_tex);
		m_cube.OnDraw();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void FBOSetup(Window& window) {
		//Create color texture for each attachment
		glCreateTextures(GL_TEXTURE_2D, 1, &m_color_tex1);
		glTextureStorage2D(m_color_tex1, 1, GL_RGBA32F, window.GetWindowDimensions().width, window.GetWindowDimensions().height);

		//Create multisampled depth texture
		glCreateTextures(GL_TEXTURE_2D, 1, &m_depth_tex);
		glTextureStorage2D(m_depth_tex, 1, GL_DEPTH_COMPONENT32, window.GetWindowDimensions().width, window.GetWindowDimensions().height);

		//Create FBO
		glGenFramebuffers(1, &m_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_color_tex1, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depth_tex, 0);

		assert(glCheckNamedFramebufferStatus(m_fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
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
#endif //FLOATING_POINT_FRAMEBUFFERS
