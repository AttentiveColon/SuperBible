#include "Defines.h"
#ifdef HIGH_QUALITY_FILTERING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* default_vertex_shader_source = R"(
#version 450 core

void main(void)
{
    const vec4 vertex[] = vec4[] ( vec4(-1.0, -1.0, 0.5, 1.0),
                                   vec4( 1.0, -1.0, 0.5, 1.0),
                                   vec4(-1.0,  1.0, 0.5, 1.0),
                                   vec4( 1.0,  1.0, 0.5, 1.0) );

    gl_Position = vertex[gl_VertexID];
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

layout (location = 0)
uniform sampler2D u_texture;

layout (location = 1)
uniform bool u_active;

out vec4 color;

vec4 hqfilter(sampler2D samp, vec2 tc)
{
	vec2 texSize = textureSize(u_texture, 0);
	vec2 uvScaled = tc * texSize + 0.5;
	vec2 uvInt = floor(uvScaled);
	vec2 uvFrac = fract(uvScaled);

	uvFrac = smoothstep(0.0, 1.0, uvFrac);
	vec2 uv = (uvInt + uvFrac - 0.5) / texSize;

	return texture(samp, uv);
}

void main()
{
	vec2 uv = vec2(gl_FragCoord.x / 1600, gl_FragCoord.y / 900);
	if (u_active)
		color = hqfilter(u_texture, uv * 0.05 + 0.2);
	else
		color = texture(u_texture, uv * 0.05 + 0.2);
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

	GLuint m_vao;
	GLuint m_program;
	GLuint m_texture;

	bool m_active = true;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		m_texture = Load_KTX("./resources/fiona.ktx");
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_program = LoadShaders(default_shader_text);
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
		glBindTextureUnit(0, m_texture);
		glUniform1i(1, m_active);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("HQ filter active", &m_active);
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
#endif //HIGH_QUALITY_FILTERING