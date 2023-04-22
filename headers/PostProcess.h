#include "GL_Helpers.h"
#include "GL/glew.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

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

out vec4 color;

in VS_OUT
{
	vec2 uv;
} fs_in;

void main()
{
	color = texture(u_texture, fs_in.uv);
}
)";

static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct PostProcess {
	GLuint m_program;
	GLuint m_fbo;
	GLuint m_texture;
	GLuint m_depth;

	GLuint m_default_program;

	//Uniform information
	GLint m_uniform_count;
	std::vector<std::string> m_uniform_names;
	std::vector<GLenum> m_uniform_types;
	std::vector<GLuint> m_uniform_locations;

	PostProcess() :m_program(0), m_fbo(0), m_texture(0), m_depth(0), m_default_program(0), m_uniform_count(0) {}
	void Init(const char* fragment_shader_filename, int window_width, int window_height);
	void StartFrame(float clear_color[4]);
	void EndFrame(float clear_color[4]);
	void ReadyFrame();
	void PresentFrame();

	void SetUniform(const char* name, void* value);
};

void PostProcess::Init(const char* fragment_shader_filename, int window_width, int window_height) {
	m_default_program = LoadShaders(default_shader_text);

	std::ifstream ifs;
	std::stringstream ss;
	ifs.open(fragment_shader_filename);

	if (!ifs.is_open()) {
		std::cerr << "Unable to open fragment shader file '" << fragment_shader_filename << "'" << std::endl;
		m_program = m_default_program;
	}
	else {
		ss << ifs.rdbuf();
		std::string frag_text = ss.str();
		const char* fragment_shader_text = frag_text.c_str();
		ShaderText pp_shader_text[] = {
			{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
			{GL_FRAGMENT_SHADER, fragment_shader_text, NULL},
			{GL_NONE, NULL, NULL}
		};

		m_program = LoadShaders(pp_shader_text);
		if (!m_program)
		{
			std::cerr << "Invalid shader program, falling back to default" << std::endl;
			m_program = m_default_program;
		}
	}

	//Determine Uniform Data
	glGetProgramiv(m_program, GL_ACTIVE_UNIFORMS, &m_uniform_count);
	for (size_t i = 0; i < m_uniform_count; ++i) {
		const GLsizei bufSize = 256;
		GLchar name[bufSize];
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveUniform(m_program, i, bufSize, &length, &size, &type, name);
		GLint location = glGetUniformLocation(m_program, name);

		std::cout << type << std::endl;
		m_uniform_names.push_back(std::string(name));
		m_uniform_types.push_back(type);
		m_uniform_locations.push_back(location);
	}
	
	if (m_fbo) return;

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

void PostProcess::ReadyFrame() {
	glUseProgram(m_program);
}

void PostProcess::PresentFrame() {
	glUseProgram(m_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTextureUnit(0, m_texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void PostProcess::SetUniform(const char* name, void* value) {
	glUseProgram(m_program);
	std::string string_name = std::string(name);
	for (size_t i = 0; i < m_uniform_names.size(); ++i) {
		if (string_name == m_uniform_names[i]) {
			switch (m_uniform_types[i]) {
			case GL_FLOAT:
				glUniform1f(m_uniform_locations[i], *(float*)value);
				return;
			case GL_INT_VEC2:
				glUniform2iv(m_uniform_locations[i], 1, (int*)value);
				return;
			default:
				std::cerr << "Unsupported Uniform Type" << std::endl;
			}

		}
	}
	std::cerr << "Uniform name: " << name << " doesn't exist or was optimized away." << std::endl;
}