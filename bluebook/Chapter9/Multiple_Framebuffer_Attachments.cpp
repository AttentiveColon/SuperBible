#include "Defines.h"
#ifdef MULTIPLE_FRAMEBUFFER_ATTACHMENTS
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

layout (location = 0)
out vec4 color0;
layout (location = 1)
out vec4 color1;
layout (location = 2)
out vec4 color2;

in VS_OUT
{
	vec3 normal;
	vec2 uv;
} fs_in;

void main()
{
	vec4 color = vec4(fs_in.uv.x, fs_in.uv.y, 0.2, 1.0);
	color0 = vec4(color.r);
	color1 = vec4(color.g);
	color2 = vec4(color.b);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* vertex_shader_source2 = R"(
#version 450 core
out VS_OUT
{	
	vec2 uv;
} vs_out;

void main()
{
	const vec2 vertices[] = 
	{
		vec2(-1.0, -1.0),
		vec2(-1.0, 1.0),
		vec2(1.0, 1.0),
		vec2(-1.0, -1.0),
		vec2(1.0, 1.0),
		vec2(1.0, -1.0)
	};
	const vec2 uv[] =
	{
		vec2(0.0, 0.0),
		vec2(0.0, 1.0),
		vec2(1.0, 1.0),
		vec2(0.0, 0.0),
		vec2(1.0, 1.0),
		vec2(1.0, 0.0)
	};
	gl_Position = vec4(vertices[gl_VertexID], 0.5, 1.0);
	vs_out.uv = uv[gl_VertexID];
}
)";

static const GLchar* fragment_shader_source2 = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D u_red;
layout (binding = 1)
uniform sampler2D u_green;
layout (binding = 2)
uniform sampler2D u_blue;

layout (location = 0)
uniform vec2 offset_red;
layout (location = 1)
uniform vec2 offset_green;
layout (location = 2)
uniform vec2 offset_blue;

out vec4 color;

in VS_OUT
{
	vec2 uv;
} fs_in;

void main()
{
	float red = texture(u_red, fs_in.uv + offset_red).r;
	float green = texture(u_green, fs_in.uv + offset_green).g;
	float blue = texture(u_blue, fs_in.uv + offset_blue).b;
	color = vec4(red, green, blue, 1.0);
}
)";

static ShaderText shader_text2[] = {
	{GL_VERTEX_SHADER, vertex_shader_source2, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source2, NULL},
	{GL_NONE, NULL, NULL}
};


struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_program2;

	GLuint m_fbo;
	GLuint m_color_textures[3];

	SB::Camera m_camera;
	bool m_input_mode = false;


	bool m_wireframe = false;
	ObjMesh m_cube;
	glm::vec3 m_cube_pos = glm::vec3(0.0f, 0.0f, 1.0f);

	glm::vec2 m_offset_red = glm::vec2(0.01, 0.0);
	glm::vec2 m_offset_green = glm::vec2(0.0, 0.01);
	glm::vec2 m_offset_blue = glm::vec2(-0.01, -0.01);


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
		m_program2 = LoadShaders(shader_text2);

		m_fbo = CreateFBO(m_color_textures, window.GetWindowDimensions().width, window.GetWindowDimensions().height);

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
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_COLOR, 1, m_clear_color);
		glClearBufferfv(GL_COLOR, 2, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_BACK);

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

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_COLOR, 1, m_clear_color);
		glClearBufferfv(GL_COLOR, 2, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program2);
		glActiveTexture(GL_TEXTURE0);
		glBindTextureUnit(0, m_color_textures[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTextureUnit(1, m_color_textures[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTextureUnit(2, m_color_textures[2]);
		glUniform2fv(0, 1, glm::value_ptr(m_offset_red));
		glUniform2fv(1, 1, glm::value_ptr(m_offset_green));
		glUniform2fv(2, 1, glm::value_ptr(m_offset_blue));
		glDrawArrays(GL_TRIANGLES, 0, 6);

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
		ImGui::DragFloat3("Cube Position", glm::value_ptr(m_cube_pos), 0.1f, -5.0f, 5.0f);
		ImGui::DragFloat2("Offset Red", glm::value_ptr(m_offset_red), 0.001, -0.2, 0.2);
		ImGui::DragFloat2("Offset Green", glm::value_ptr(m_offset_green), 0.001, -0.2, 0.2);
		ImGui::DragFloat2("Offset Blue", glm::value_ptr(m_offset_blue), 0.001, -0.2, 0.2);
		ImGui::End();
	}

	GLuint CreateFBO(GLuint color_textures[3], int window_width, int window_height) {
		static const GLenum draw_buffers[] = {
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2
		};
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glGenTextures(3, &color_textures[0]);

		for (int i = 0; i < 3; ++i) {
			glBindTexture(GL_TEXTURE_2D, color_textures[i]);
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, window_width, window_height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glFramebufferTexture(GL_FRAMEBUFFER, draw_buffers[i], color_textures[i], 0);
		}
		GLuint depth_texture;
		glGenTextures(1, &depth_texture);
		glBindTexture(GL_TEXTURE_2D, depth_texture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, window_width, window_height);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture, 0);
		glDrawBuffers(3, draw_buffers);
		return fbo;
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
#endif //MULTIPLE_FRAMEBUFFER_ATTACHMENTS
