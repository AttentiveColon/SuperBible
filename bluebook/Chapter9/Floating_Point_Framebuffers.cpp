#include "Defines.h"
#ifdef FLOATING_POINT_FRAMEBUFFERS
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

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

static const GLchar* adaptive_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D u_texture;

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
	float lum[25];
	vec2 tex_scale = vec2(1.0) / textureSize(u_texture, 0);

	for (int i = 0; i < 25; ++i)
	{
		vec2 tc = (2.0 * gl_FragCoord.xy + 3.5 * vec2(i % 5 - 2, 5 - 2));
		vec3 col = texture(u_texture, tc * tex_scale).rgb;
		lum[i] = dot(col, vec3(0.3, 0.59, 0.11));
	}

	vec3 vColor = texelFetch(u_texture, ivec2(gl_FragCoord.xy), 0).rgb;

	float kernelLuminance = (
          (1.0  * (lum[0] + lum[4] + lum[20] + lum[24])) +
          (4.0  * (lum[1] + lum[3] + lum[5] + lum[9] +
                  lum[15] + lum[19] + lum[21] + lum[23])) +
          (7.0  * (lum[2] + lum[10] + lum[14] + lum[22])) +
          (16.0 * (lum[6] + lum[8] + lum[16] + lum[18])) +
          (26.0 * (lum[7] + lum[11] + lum[13] + lum[17])) +
          (41.0 * lum[12])
          ) / 273.0;


	float exposure = sqrt(8.0 / (kernelLuminance + 0.25));

	color.rgb = 1.0 - exp2(-vColor * exposure);
	color.a = 1.0f;
}
)";


static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static ShaderText adaptive_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, adaptive_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program2, m_adaptive_shader;
	GLuint m_vao;


	GLuint m_tex;


	GLuint m_fbo, m_color_tex1, m_depth_tex;
	bool m_active = true;
	float m_exposure = 1.0f;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program2 = LoadShaders(default_shader_text);
		m_adaptive_shader = LoadShaders(adaptive_shader_text);
		m_tex = Load_KTX("./resources/treelights_2k.ktx");

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		//Create FBO with Floating point buffers
		FBOSetup(window);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const GLfloat one = 1.0f;


		//Bind framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		static const GLuint draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, draw_buffers);

		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		//Render scene to floating point buffer textures
		RenderScene(m_program2, m_tex);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);
		glDisable(GL_DEPTH_TEST);
		glClearBufferfv(GL_COLOR, 0, m_clear_color);


		//Render texture with adaptive hdr
		if (m_active)
			RenderScene(m_adaptive_shader, m_color_tex1);
		else RenderScene(m_program2, m_color_tex1);

		glBindTextureUnit(0, 0);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Shader Active", &m_active);
		ImGui::DragFloat("Exposure", &m_exposure, 0.1f, 0.01f, 20.0f);
		ImGui::End();
	}

	void RenderScene(GLuint shader, GLuint texture) {
		glUseProgram(shader);
		glBindTextureUnit(0, texture);
		glUniform1i(4, m_active);
		glUniform1f(1, m_exposure);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void FBOSetup(Window& window) {
		//Create color texture for floating point buffer attachment
		glCreateTextures(GL_TEXTURE_2D, 1, &m_color_tex1);
		glTextureStorage2D(m_color_tex1, 1, GL_RGBA32F, window.GetWindowDimensions().width, window.GetWindowDimensions().height);

		//Create depth texture
		glCreateTextures(GL_TEXTURE_2D, 1, &m_depth_tex);
		glTextureStorage2D(m_depth_tex, 1, GL_DEPTH_COMPONENT32, window.GetWindowDimensions().width, window.GetWindowDimensions().height);

		//Create FBO
		glGenFramebuffers(1, &m_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_color_tex1, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depth_tex, 0);

		assert(glCheckNamedFramebufferStatus(m_fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
