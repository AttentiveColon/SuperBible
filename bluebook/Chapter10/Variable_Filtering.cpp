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

layout (location = 4)
uniform vec3 model_pos;
layout (location = 5)
uniform mat4 u_view;
layout (location = 6)
uniform mat4 u_proj;

out vec2 tc;
out vec3 V;

void main()
{
	vec4 P = u_view * vec4(position + model_pos, 1.0);


	gl_Position = u_proj * P;
	V = -P.xyz;
	tc = uv;
}
)";

static const GLchar* render_fragment_shader_source = R"(
#version 450 core

in vec2 tc;
in vec3 V;

out vec4 color;

void main()
{
	color = vec4(tc.x, tc.y, 0.0, V.z);
}
)";

static ShaderText render_shader_text[] = {
	{GL_VERTEX_SHADER, render_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, render_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

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
#version 430 core

layout (binding = 0) uniform sampler2D input_image;

layout (location = 0) out vec4 color;

layout (location = 1)
uniform float focal_distance;
layout (location = 2)
uniform float focal_depth;

void main(void)
{
    vec2 s = 1.0 / textureSize(input_image, 0);
    vec2 C = gl_FragCoord.xy;

    vec4 v = texelFetch(input_image, ivec2(gl_FragCoord.xy), 0).rgba;

    float m;

    if (v.w == 0.0)
    {
        m = 0.5;
    }
    else
    {
        m = abs(v.w - focal_distance);
        m = 0.5 + smoothstep(0.0, focal_depth, m) * 6.5;
    }

    vec2 P0 = vec2(C * 1.0) + vec2(-m, -m);
    vec2 P1 = vec2(C * 1.0) + vec2(-m, m);
    vec2 P2 = vec2(C * 1.0) + vec2(m, -m);
    vec2 P3 = vec2(C * 1.0) + vec2(m, m);

    P0 *= s;
    P1 *= s;
    P2 *= s;
    P3 *= s;

    vec3 a = textureLod(input_image, P0, 0).rgb;
    vec3 b = textureLod(input_image, P1, 0).rgb;
    vec3 c = textureLod(input_image, P2, 0).rgb;
    vec3 d = textureLod(input_image, P3, 0).rgb;

    vec3 f = a - b - c + d;

    m *= 2;

    f /= float(m * m);

    color = vec4(f, 1.0);
}
)";

static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* compute_shader_source = R"(
#version 450 core

layout (local_size_x = 1024) in;

layout (binding = 0, rgba32f) readonly uniform image2D input_image;
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

	GLuint m_render_program, m_display_program, m_compute_shader, m_bypass_program;

	GLuint m_depth_fbo, m_depth_tex, m_color_tex, m_temp_tex;

	GLuint m_texture, m_summation_texture;

	bool m_input_mode = false;

	float m_focal_distance = 19.0f;
	float m_focal_depth = 23.0f;

	ObjMesh m_cube;
	glm::vec3 m_cube_pos[3] = {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-2.0f, 0.0f, -12.0f), glm::vec3(-4.0f, 0.0f, -24.0f)};
	
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
		m_camera = SB::Camera("Camera", glm::vec3(-3.0f, 2.0f, 10.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);

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
		glUseProgram(m_compute_shader);
		glBindImageTexture(0, m_color_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, m_temp_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		glDispatchCompute(900, 1, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindImageTexture(0, m_temp_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, m_color_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		glDispatchCompute(1600, 1, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//display fbo
		glBindTextureUnit(0, m_color_tex);
		glDisable(GL_DEPTH_TEST);
		glUseProgram(m_display_program);
		glUniform1f(1, m_focal_distance);
		glUniform1f(2, m_focal_depth);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat("Focal Distance", &m_focal_distance, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat("Focal Depth", &m_focal_depth, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat3("Cube Pos", glm::value_ptr(m_cube_pos[0]), 0.1f, -100.0f, 100.0f);
		ImGui::End();
	}

	void SetupDepthFBO() {
		glGenFramebuffers(1, &m_depth_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_depth_fbo);

		glGenTextures(1, &m_depth_tex);
		glBindTexture(GL_TEXTURE_2D, m_depth_tex);
		glTexStorage2D(GL_TEXTURE_2D, 11, GL_DEPTH_COMPONENT32F, 2048, 2048);
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


		glViewport(0, 0, 1600, 900);
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glUseProgram(m_render_program);
		glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(6, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));

		glClearBufferfv(GL_DEPTH, 0, &one);

		for (int i = 0; i < 3; ++i) {
			glUniform3fv(4, 1, glm::value_ptr(m_cube_pos[i]));
			m_cube.OnDraw();
		}

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
