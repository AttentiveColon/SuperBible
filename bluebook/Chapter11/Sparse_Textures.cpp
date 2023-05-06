#include "Defines.h"
#ifdef SPARSE_TEXTURES
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* default_vertex_shader_source = R"(
#version 440 core

out vec2 uv;

void main(void)
{
    vec2 pos = vec2(float(gl_VertexID & 2), float(gl_VertexID * 2 & 2));
    uv = pos * 0.5;
    gl_Position = vec4(pos - vec2(1.0), 0.0, 1.0);
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 440 core

layout (binding = 0) uniform sampler2D sparseTex;

layout (binding = 0, std140) uniform TEXURE_BLOCK
{
    uint foo;
};

in vec2 uv;

layout (location = 0) out vec4 o_color;

void main(void)
{
    // o_color = vec4(0.3, 0.6, 0.0, 1.0);
    o_color = textureLod(sparseTex, uv, 0.0);
}
)";

static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

void GetSparsePageInfo() {
	GLint num_page_sizes;
	GLint page_sizes_x[10];
	GLint page_sizes_y[10];
	GLint page_sizes_z[10];

	glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, sizeof(GLuint), &num_page_sizes);

	num_page_sizes = min(num_page_sizes, 10);

	glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_X_ARB, num_page_sizes * sizeof(GLuint), page_sizes_x);
	glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Y_ARB, num_page_sizes * sizeof(GLuint), page_sizes_y);
	glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, GL_VIRTUAL_PAGE_SIZE_Z_ARB, num_page_sizes * sizeof(GLuint), page_sizes_z);

	for (int i = 0; i < num_page_sizes; ++i) {
		std::cout << i << ". X: " << page_sizes_x[i] << " Y: " << page_sizes_y[i] << " Z: " << page_sizes_z[i] << std::endl;
	}
}

const unsigned char bit_reversal_table[256] =
{
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

enum
{
	PAGE_SIZE = 128,
	TEX_SIZE = 16 * PAGE_SIZE
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_vao;
	GLuint m_program, m_compute_shader;

	GLuint m_texture;

	unsigned char* texture_data;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		FILE* f;
		GetSparsePageInfo();

		m_program = LoadShaders(default_shader_text);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
		glTexStorage2D(GL_TEXTURE_2D, 11, GL_RGBA8, TEX_SIZE, TEX_SIZE);

		glGenVertexArrays(1, &m_vao);

		texture_data = new unsigned char[PAGE_SIZE * PAGE_SIZE * 4];
		f = fopen("./resources/smiley.raw", "rb");
		fread(texture_data, 128 * 128 * 4, 1, f);
		fclose(f);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		float f = (float)m_time;

		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glBindVertexArray(m_vao);
		glUseProgram(m_program);

		glBindTexture(GL_TEXTURE_2D, m_texture);

		static int t = 0;
		int r = (t & 0x100);
		int tile = bit_reversal_table[(t & 0xFF)];

		int x = (tile >> 0) & 0xF;
		int y = (tile >> 4) & 0xF;

		if (r == 0) {
			glTexPageCommitmentARB(GL_TEXTURE_2D, 0, x * PAGE_SIZE, y * PAGE_SIZE, 0, PAGE_SIZE, PAGE_SIZE, 1, GL_TRUE);
			glTexSubImage2D(GL_TEXTURE_2D, 0, x * PAGE_SIZE, y * PAGE_SIZE, PAGE_SIZE, PAGE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
		}
		else {
			glTexPageCommitmentARB(GL_TEXTURE_2D, 0, x * PAGE_SIZE, y * PAGE_SIZE, 0, PAGE_SIZE, PAGE_SIZE, 1, GL_FALSE);
		}
		t += 17;

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
#endif //SPARSE_TEXTURES