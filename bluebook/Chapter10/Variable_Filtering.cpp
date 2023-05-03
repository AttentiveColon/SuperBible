#include "Defines.h"
#ifdef VARIABLE_FILTERING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* render_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 5)
uniform mat4 mvp;

out vec2 tc;

void main()
{
	gl_Position = mvp * vec4(position, 1.0);
	tc = uv;
}
)";

static const GLchar* render_fragment_shader_source = R"(
#version 450 core

in vec2 tc;

out vec4 color;

void main()
{
	color = vec4(tc.x, tc.y, 0.0, 1.0);
}
)";

static ShaderText render_shader_text[] = {
	{GL_VERTEX_SHADER, render_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, render_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

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
	color = texture(u_texture, uv);
}
)";

static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* compute_shader_source = R"(
#version 450 core

layout (local_size_x = 724) in;

layout (binding = 0, rgba8) readonly uniform image2D input_image;
layout (binding = 1, rgba32f) writeonly uniform image2D output_image;

shared vec3 shared_data[gl_WorkGroupSize.x * 2];

void main()
{
	uint id = gl_LocalInvocationID.x;
	uint rd_id;
	uint wr_id;
	uint mask;
	ivec2 P0 = ivec2(id * 2, gl_WorkGroupID.x);
	ivec2 P1 = ivec2(id * 2 + 1, gl_WorkGroupID.x);

	const uint steps = uint(log2(gl_WorkGroupSize.x)) + 1;
	uint step = 0;
	
	vec4 i0 = imageLoad(input_image, P0);
	vec4 i1 = imageLoad(input_image, P1);

	shared_data[P0.x] = i0.rgb;
	shared_data[P1.x] = i1.rgb;

	barrier();
	
	for (step = 0; step < steps; ++step)
	{
		mask = (1 << step) - 1;
		rd_id = ((id >> step) << (step + 1)) + mask;
		wr_id = rd_id + 1 + (id & mask);

		shared_data[wr_id] += shared_data[rd_id];

		barrier();
	}
	
	imageStore(output_image, P0.yx, vec4(shared_data[P0.x], i0.a));
	imageStore(output_image, P1.yx, vec4(shared_data[P1.x], i1.a));

	//ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
	//vec4 texel = imageLoad(input_image, uv);
	//imageStore(output_image, uv, texel);
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

	GLuint m_render_program, m_display_program, m_compute_shader;

	GLuint m_depth_fbo, m_depth_tex, m_color_tex, m_temp_tex;

	GLuint m_texture, m_summation_texture;

	bool m_original_texture_active = false;
	bool m_input_mode = false;

	ObjMesh m_cube;
	SB::Camera m_camera;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		m_render_program = LoadShaders(render_shader_text);
		m_display_program = LoadShaders(default_shader_text);
		m_compute_shader = LoadShaders(compute_shader_text);

		m_cube.Load_OBJ("./resources/cube.obj");
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 1.0f, -5.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);

		SetupDepthFBO();


		
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
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		//render scene into fbo
		RenderScene();

		//compute rendered scene
		/*glUseProgram(m_compute_shader);
		glBindImageTexture(0, m_color_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, m_temp_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glDispatchCompute(2048, 1, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindImageTexture(0, m_temp_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, m_color_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		glDispatchCompute(2048, 1, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);*/


		//display fbo
		glUseProgram(m_display_program);
		glBindTextureUnit(0, m_color_tex);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Original Texture", &m_original_texture_active);
		ImGui::End();
	}

	void SetupDepthFBO() {
		glGenFramebuffers(1, &m_depth_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_depth_fbo);

		glGenTextures(1, &m_depth_tex);
		glBindTexture(GL_TEXTURE_2D, m_depth_tex);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 2048, 2048);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenTextures(1, &m_color_tex);
		glBindTexture(GL_TEXTURE_2D, m_color_tex);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 2048, 2048);

		glGenTextures(1, &m_temp_tex);
		glBindTexture(GL_TEXTURE_2D, m_temp_tex);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 2048, 2048);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depth_tex, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_color_tex, 0);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glEnable(GL_DEPTH_TEST);
	}

	void RenderScene() {
		static const GLfloat one = 1.0f;
		static const GLenum attachments[] = { GL_COLOR_ATTACHMENT0 };

		glEnable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_FRAMEBUFFER, m_depth_fbo);
		glDrawBuffers(1, attachments);

		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glUseProgram(m_render_program);
		glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		m_cube.OnDraw();

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
#endif //VARIABLE_FILTERING
