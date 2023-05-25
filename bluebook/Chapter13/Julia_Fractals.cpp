#include "Defines.h"
#ifdef JULIA_FRACTALS
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

out VS_OUT
{
	vec2 uv;
} vs_out;

void main(void)
{
    const vec4 vertices[] = vec4[]( vec4(-1.0, -1.0, 0.5, 1.0),
                                    vec4( 1.0, -1.0, 0.5, 1.0),
                                    vec4(-1.0,  1.0, 0.5, 1.0),
                                    vec4( 1.0,  1.0, 0.5, 1.0) );

	const vec2 texture_coordinates[] = vec2[]( vec2(0.0, 0.0),
											   vec2(1.0, 0.0),
											   vec2(0.0, 1.0),
											   vec2(1.0, 1.0) );

	vs_out.uv = texture_coordinates[gl_VertexID];
    gl_Position = vertices[gl_VertexID];
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in VS_OUT
{
	vec2 uv;
} fs_in;

layout (location = 0)
uniform vec2 c = vec2(0.2, 0.5);
layout (location = 1)
uniform int max_iterations = 100;

layout (binding = 0)
uniform sampler1D tex_gradient;

out vec4 color;

void main()
{
	int iterations = 0;
	vec2 z = fs_in.uv;
	const float threshold_squared = 4.0;
	
	while (iterations < max_iterations && dot(z, z) < threshold_squared)
	{
		vec2 z_squared;
		z_squared.x = z.x * z.x - z.y * z.y;
		z_squared.y = 2.0 * z.x * z.y;
		z = z_squared + c;
		iterations++;
	}

	if (iterations == max_iterations)
	{
		color = vec4(0.0);
	}
	else
	{
		color = texture(tex_gradient, float(iterations) / float(max_iterations));
	}
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

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);
		glEnable(GL_DEPTH_TEST);

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		static const GLubyte tex_gradient_data[] = {
            0x00, 0x00, 0x00, 0xFF, 0x0E, 0x03, 0xFF, 0x1C,
            0x07, 0xFF, 0x2A, 0x0A, 0xFF, 0x38, 0x0E, 0xFF,
            0x46, 0x12, 0xFF, 0x54, 0x15, 0xFF, 0x62, 0x19,
            0xFF, 0x71, 0x1D, 0xFF, 0x7F, 0x20, 0xFF, 0x88,
            0x22, 0xFF, 0x92, 0x25, 0xFF, 0x9C, 0x27, 0xFF,
            0xA6, 0x2A, 0xFF, 0xB0, 0x2C, 0xFF, 0xBA, 0x2F,
            0xFF, 0xC4, 0x31, 0xFF, 0xCE, 0x34, 0xFF, 0xD8,
            0x36, 0xFF, 0xE2, 0x39, 0xFF, 0xEC, 0x3B, 0xFF,
            0xF6, 0x3E, 0xFF, 0xFF, 0x40, 0xF8, 0xFE, 0x40,
            0xF1, 0xFE, 0x40, 0xEA, 0xFE, 0x41, 0xE3, 0xFD,
            0x41, 0xDC, 0xFD, 0x41, 0xD6, 0xFD, 0x42, 0xCF,
            0xFC, 0x42, 0xC8, 0xFC, 0x42, 0xC1, 0xFC, 0x43,
            0xBA, 0xFB, 0x43, 0xB4, 0xFB, 0x43, 0xAD, 0xFB,
            0x44, 0xA6, 0xFA, 0x44, 0x9F, 0xFA, 0x45, 0x98,
            0xFA, 0x45, 0x92, 0xF9, 0x45, 0x8B, 0xF9, 0x46,
            0x84, 0xF9, 0x46, 0x7D, 0xF8, 0x46, 0x76, 0xF8,
            0x46, 0x6F, 0xF8, 0x47, 0x68, 0xF8, 0x47, 0x61,
            0xF7, 0x47, 0x5A, 0xF7, 0x48, 0x53, 0xF7, 0x48,
            0x4C, 0xF6, 0x48, 0x45, 0xF6, 0x49, 0x3E, 0xF6,
            0x49, 0x37, 0xF6, 0x4A, 0x30, 0xF5, 0x4A, 0x29,
            0xF5, 0x4A, 0x22, 0xF5, 0x4B, 0x1B, 0xF5, 0x4B,
            0x14, 0xF4, 0x4B, 0x0D, 0xF4, 0x4C, 0x06, 0xF4,
            0x4D, 0x04, 0xF1, 0x51, 0x0D, 0xE9, 0x55, 0x16,
            0xE1, 0x59, 0x1F, 0xD9, 0x5D, 0x28, 0xD1, 0x61,
            0x31, 0xC9, 0x65, 0x3A, 0xC1, 0x69, 0x42, 0xB9,
            0x6D, 0x4B, 0xB1, 0x71, 0x54, 0xA9, 0x75, 0x5D,
            0xA1, 0x79, 0x66, 0x99, 0x7D, 0x6F, 0x91, 0x81,
            0x78, 0x89, 0x86, 0x80, 0x81, 0x8A, 0x88, 0x7A,
            0x8E, 0x90, 0x72, 0x92, 0x98, 0x6A, 0x96, 0xA1,
            0x62, 0x9A, 0xA9, 0x5A, 0x9E, 0xB1, 0x52, 0xA2,
            0xBA, 0x4A, 0xA6, 0xC2, 0x42, 0xAA, 0xCA, 0x3A,
            0xAE, 0xD3, 0x32, 0xB2, 0xDB, 0x2A, 0xB6, 0xE3,
            0x22, 0xBA, 0xEB, 0x1A, 0xBE, 0xF4, 0x12, 0xC2,
            0xFC, 0x0A, 0xC6, 0xF5, 0x02, 0xCA, 0xE6, 0x09,
            0xCE, 0xD8, 0x18, 0xD1, 0xCA, 0x27, 0xD5, 0xBB,
            0x36, 0xD8, 0xAD, 0x45, 0xDC, 0x9E, 0x54, 0xE0,
            0x90, 0x62, 0xE3, 0x82, 0x6F, 0xE6, 0x71, 0x7C,
            0xEA, 0x61, 0x89, 0xEE, 0x51, 0x96, 0xF2, 0x40,
            0xA3, 0xF5, 0x30, 0xB0, 0xF9, 0x20, 0xBD, 0xFD,
            0x0F, 0xE3, 0xFF, 0x01, 0xE9, 0xFF, 0x01, 0xEE,
            0xFF, 0x01, 0xF4, 0xFF, 0x00, 0xFA, 0xFF, 0x00,
            0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x0A, 0xFF, 0xFF,
            0x15, 0xFF, 0xFF, 0x20, 0xFF, 0xFF, 0x2B, 0xFF,
            0xFF, 0x36, 0xFF, 0xFF, 0x41, 0xFF, 0xFF, 0x4C,
            0xFF, 0xFF, 0x57, 0xFF, 0xFF, 0x62, 0xFF, 0xFF,
            0x6D, 0xFF, 0xFF, 0x78, 0xFF, 0xFF, 0x81, 0xFF,
            0xFF, 0x8A, 0xFF, 0xFF, 0x92, 0xFF, 0xFF, 0x9A,
            0xFF, 0xFF, 0xA3, 0xFF, 0xFF, 0xAB, 0xFF, 0xFF,
            0xB4, 0xFF, 0xFF, 0xBC, 0xFF, 0xFF, 0xC4, 0xFF,
            0xFF, 0xCD, 0xFF, 0xFF, 0xD5, 0xFF, 0xFF, 0xDD,
            0xFF, 0xFF, 0xE6, 0xFF, 0xFF, 0xEE, 0xFF, 0xFF,
            0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF9,
            0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xED, 0xFF, 0xFF,
            0xE7, 0xFF, 0xFF, 0xE1, 0xFF, 0xFF, 0xDB, 0xFF,
            0xFF, 0xD5, 0xFF, 0xFF, 0xCF, 0xFF, 0xFF, 0xCA,
            0xFF, 0xFF, 0xC4, 0xFF, 0xFF, 0xBE, 0xFF, 0xFF,
            0xB8, 0xFF, 0xFF, 0xB2, 0xFF, 0xFF, 0xAC, 0xFF,
            0xFF, 0xA6, 0xFF, 0xFF, 0xA0, 0xFF, 0xFF, 0x9B,
            0xFF, 0xFF, 0x96, 0xFF, 0xFF, 0x90, 0xFF, 0xFF,
            0x8B, 0xFF, 0xFF, 0x86, 0xFF, 0xFF, 0x81, 0xFF,
            0xFF, 0x7B, 0xFF, 0xFF, 0x76, 0xFF, 0xFF, 0x71,
            0xFF, 0xFF, 0x6B, 0xFF, 0xFF, 0x66, 0xFF, 0xFF,
            0x61, 0xFF, 0xFF, 0x5C, 0xFF, 0xFF, 0x56, 0xFF,
            0xFF, 0x51, 0xFF, 0xFF, 0x4C, 0xFF, 0xFF, 0x47,
            0xFF, 0xFF, 0x41, 0xF9, 0xFF, 0x40, 0xF0, 0xFF,
            0x40, 0xE8, 0xFF, 0x40, 0xDF, 0xFF, 0x40, 0xD7,
            0xFF, 0x40, 0xCF, 0xFF, 0x40, 0xC6, 0xFF, 0x40,
            0xBE, 0xFF, 0x40, 0xB5, 0xFF, 0x40, 0xAD, 0xFF,
            0x40, 0xA4, 0xFF, 0x40, 0x9C, 0xFF, 0x40, 0x95,
            0xFF, 0x40, 0x8D, 0xFF, 0x40, 0x86, 0xFF, 0x40,
            0x7E, 0xFF, 0x40, 0x77, 0xFF, 0x40, 0x6F, 0xFF,
            0x40, 0x68, 0xFF, 0x40, 0x60, 0xFF, 0x40, 0x59,
            0xFF, 0x40, 0x51, 0xFF, 0x40, 0x4A, 0xFA, 0x43,
            0x42, 0xF3, 0x48, 0x3E, 0xED, 0x4E, 0x3D, 0xE6,
            0x53, 0x3B, 0xDF, 0x58, 0x39, 0xD8, 0x5E, 0x37,
            0xD2, 0x63, 0x35, 0xCB, 0x68, 0x34, 0xC4, 0x6D,
            0x32, 0xBD, 0x73, 0x30, 0xB7, 0x78, 0x2E, 0xB0,
            0x7D, 0x2D, 0xA9, 0x83, 0x2B, 0xA2, 0x88, 0x29,
            0x9C, 0x8D, 0x27, 0x95, 0x92, 0x25, 0x8E, 0x98,
            0x24, 0x87, 0x9D, 0x22, 0x81, 0xA2, 0x20, 0x7A,
            0xA6, 0x1E, 0x74, 0xAB, 0x1D, 0x6E, 0xB0, 0x1B,
            0x68, 0xB5, 0x1A, 0x61, 0xB9, 0x18, 0x5B, 0xBE,
            0x17, 0x55, 0xC3, 0x15, 0x4F, 0xC8, 0x13, 0x48,
            0xCD, 0x12, 0x42, 0xD1, 0x10, 0x3C, 0xD6, 0x0F,
            0x36, 0xDB, 0x0D, 0x2F, 0xE0, 0x0C, 0x29, 0xE4,
            0x0A, 0x23, 0xE9, 0x08, 0x1D, 0xEE, 0x07, 0x16,
            0xF3, 0x05, 0x10, 0xF7, 0x04, 0x0A, 0xFC, 0x02,
            0x04, 0xFB, 0x01, 0x04, 0xEF, 0x00, 0x12, 0xE4,
            0x00, 0x1F, 0xD9, 0x00, 0x2D, 0xCE, 0x00, 0x3B,
            0xC2, 0x00, 0x48, 0xB7, 0x00, 0x56, 0xAC, 0x00,
            0x64, 0xA0, 0x00, 0x72, 0x95, 0x00, 0x7F, 0x8A,
            0x00, 0x88, 0x7F, 0x00, 0x92, 0x75, 0x00, 0x9C,
            0x6B, 0x00, 0xA6, 0x61, 0x00, 0xB0, 0x57, 0x00,
            0xBA, 0x4E, 0x00, 0xC4, 0x44, 0x00, 0xCE, 0x3A,
            0x00, 0xD8, 0x30, 0x00, 0xE2, 0x27, 0x00, 0xEC,
            0x1D, 0x00, 0xF6, 0x13, 0x00, 0xFF, 0x09, 0x00,
		};

		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_1D, texture);
		glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGB8, sizeof(tex_gradient_data) / 4);
		glTexSubImage1D(GL_TEXTURE_1D, 0, 0, sizeof(tex_gradient_data) / 4, GL_RGBA, GL_UNSIGNED_BYTE, tex_gradient_data);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_1D, 0);

		glBindTextureUnit(0, texture);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float time = float(m_time * 0.1f);
        float offset = glm::fract(float(m_time * 0.01f));
		
		glm::vec2 c = glm::vec2(sin(time + offset) * cos(time), cos(time + offset));

		glUseProgram(m_program);
		glUniform2fv(0, 1, glm::value_ptr(c));
		glUniform1i(1, 1000);
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
#endif //JULIA_FRACTALS