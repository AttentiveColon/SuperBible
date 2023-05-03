#include "Defines.h"
#ifdef VARIABLE_FILTERING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* default_vertex_shader_source = R"(
#version 450 core

out vec2 uv;

void main()
{
	const vec2 vertices[] = 
	{
		vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0),
		vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(1.0, -1.0)
	};
	const vec2 uvs[] = 
	{
		vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0),
		vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 0.0)
	};
	gl_Position = vec4(vertices[gl_VertexID], 0.5, 1.0);
	uv = uvs[gl_VertexID];
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D u_texture;

in vec2 uv;

out vec4 color;

void main()
{
	color = texture(u_texture, vec2(uv.x, -uv.y));
}
)";

static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* compute_shader_source = R"(
#version 450 core

layout (local_size_x = 16) in;

layout (binding = 0, rgba8) readonly uniform image2D input_image;
layout (binding = 1, rgba32f) writeonly uniform image2D output_image;

const uint array_size = (gl_WorkGroupSize.x * gl_NumWorkGroups.x) * 2;

shared vec4 shared_data[array_size];

void main()
{
	uint id = gl_GlobalInvocationID.x;
	uint rd_id;
	uint wr_id;
	uint mask;
	ivec2 P = ivec2(id * 2, gl_WorkGroupID.y);
	
	

	ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	vec4 texel = imageLoad(input_image, uv);
	imageStore(output_image, uv, texel);
}
)";

static ShaderText compute_shader_text[] = {
	{GL_COMPUTE_SHADER, compute_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct TextureDimensions {
	int width;
	int height;
};

TextureDimensions get_dimensions(GLuint tex) {
	TextureDimensions dimensions;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &dimensions.width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &dimensions.height);
	glBindTexture(GL_TEXTURE_2D, 0);
	return dimensions;
}

GLint get_internal_format(GLuint tex) {
	GLint format;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	glBindTexture(GL_TEXTURE_2D, 0);
	return format;
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program, m_compute_shader;

	GLuint m_texture, m_summation_texture;
	

	Random m_rand;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		m_rand.Init();
		m_program = LoadShaders(default_shader_text);
		m_compute_shader = LoadShaders(compute_shader_text);
		m_texture = Load_KTX("./resources/trees.ktx");
		TextureDimensions dimensions = get_dimensions(m_texture);

		std::cout << "Width: " << dimensions.width << "\nHeight: " << dimensions.height << std::endl;

		glGenTextures(1, &m_summation_texture);
		glBindTexture(GL_TEXTURE_2D, m_summation_texture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, dimensions.width, dimensions.height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		GLint format = get_internal_format(m_texture);
		glUseProgram(m_compute_shader);
		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_ONLY, format);
		glBindImageTexture(1, m_summation_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glDispatchCompute(dimensions.width / 16, dimensions.height, 1);
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
		glBindTextureUnit(0, m_summation_texture);
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
#endif //VARIABLE_FILTERING
