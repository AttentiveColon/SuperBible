#include "Defines.h"
#ifdef COMPUTER_SHADER_INPUTS_OUTPUTS
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* default_vertex_shader_source = R"(
#version 450 core

void main()
{
	const vec2 vertices[] = 
	{
		vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0),
		vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(1.0, -1.0)
	};
	gl_Position = vec4(vertices[gl_VertexID], 0.5, 1.0);
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D u_texture;


out vec4 color;

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

static const GLchar* compute_shader_source = R"(
#version 450 core

layout (local_size_x = 32, local_size_y = 32) in;

layout (binding = 0, rgba8)
readonly uniform image2D img_input;

layout (binding = 1)
writeonly uniform image2D img_output;

void main()
{
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);

	vec4 texel = imageLoad(img_input, p);
	texel = vec4(1.0) - texel;
	imageStore(img_output, p, texel);
}
)";

static ShaderText compute_shader_text[] = {
	{GL_COMPUTE_SHADER, compute_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program, m_compute_shader;
	GLuint m_vao;

	GLuint m_tex, m_tex_out;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(default_shader_text);
		m_compute_shader = LoadShaders(compute_shader_text);
		m_tex = Load_KTX("./resources/fiona.ktx");

		glGenTextures(1, &m_tex_out);
		glBindTexture(GL_TEXTURE_2D, m_tex_out);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 3648, 2736);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		glUseProgram(m_compute_shader);
		glBindImageTexture(0, m_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		glBindImageTexture(1, m_tex_out, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		glDispatchCompute(3648 / 32, 2736 / 32, 1);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {


		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glBindTextureUnit(0, m_tex_out);
		glDrawArrays(GL_TRIANGLES, 0, 6);
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
#endif //COMPUTER_SHADER_INPUTS_OUTPUTS
