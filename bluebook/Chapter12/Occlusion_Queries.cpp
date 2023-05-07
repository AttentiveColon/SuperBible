#include "Defines.h"
#ifdef OCCLUSION_QUERIES
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* occlusion_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 4)
uniform mat4 u_viewProj;

layout (location = 5)
uniform vec3 u_model;

layout (location = 6)
uniform vec3 u_color;


out VS_OUT
{
	vec3 normal;
	vec2 uv;
	vec3 color;
} vs_out;

void main(void)
{
    gl_Position = u_viewProj * vec4(position + u_model, 1.0);
	vs_out.normal = normal;
	vs_out.uv = uv;
	vs_out.color = u_color;
}
)";

static const GLchar* occlusion_fragment_shader_source = R"(
#version 450 core

in VS_OUT
{
	vec3 normal;
	vec2 uv;
	vec3 color;
} fs_in;

out vec4 color;

void main()
{
	color = vec4(fs_in.color, 1.0);
}
)";

static ShaderText occlusion_shader_text[] = {
	{GL_VERTEX_SHADER, occlusion_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, occlusion_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* default_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 4)
uniform mat4 u_viewProj;

layout (location = 5)
uniform uint u_index;

layout (binding = 0) uniform DefaultUniform
{
	vec4 cube_pos[64];
};

out VS_OUT
{
	vec3 normal;
	vec2 uv;
} vs_out;

void main(void)
{
    gl_Position = u_viewProj * (cube_pos[u_index] + vec4(position, 1.0));
	vs_out.normal = normal;
	vs_out.uv = uv;
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

in VS_OUT
{
	vec3 normal;
	vec2 uv;
} fs_in;

out vec4 color;

void main()
{
	color = vec4(fs_in.uv.x, fs_in.uv.y, 0.0, 1.0);
}
)";

static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

#define NUM_CUBES 64

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program, m_occlusion_program;
	GLuint m_ubo;

	GLuint m_queries[NUM_CUBES];

	ObjMesh m_cube;
	glm::vec4 m_cube_positions[NUM_CUBES];

	SB::Camera m_camera;
	bool m_input_mode = false;

	Random m_random;

	int m_rendered_cube_count;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(default_shader_text);
		m_occlusion_program = LoadShaders(occlusion_shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 2.0f, -12.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_cube.Load_OBJ("./resources/cube.obj");
		m_random.Init();

		glGenQueries(NUM_CUBES, m_queries);

		glGenBuffers(1, &m_ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(m_cube_positions), nullptr, GL_STATIC_DRAW);
		
		glm::vec4* positions = (glm::vec4*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(m_cube_positions), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		for (int i = 0; i < NUM_CUBES; ++i) {
			positions[i] = glm::vec4(m_random.Float() * 25.0f, m_random.Float() * 25.0f, m_random.Float() * 25.0f, 0.0f);
		}

		glUnmapBuffer(GL_UNIFORM_BUFFER);
		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
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

		//First render a cube to the scene that will occlude the other cubes
		RenderOcclusionCube();

		//Disable color and depth rendering, render our "simple" version of objects and query if they pass
		//depth and stencil sampling
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		for (unsigned int i = 0; i < NUM_CUBES; ++i) {
			glBeginQuery(GL_SAMPLES_PASSED, m_queries[i]);
			RenderBasicScene(i);
			glEndQuery(GL_SAMPLES_PASSED);
		}

		//call this so our queries are actually availabe for this example, avoid using glFinish normally
		glFlush();
		glFinish();

		//Turn on color and depth rendering, Begin condition rendering based on the result of out previous queries
		//Our render complex scene function will only make opengl calls if the related query is true
		//We get the result again in RenderComplexScene so we can count how many cubes passed the query and were rendered
		m_rendered_cube_count = 0;
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
		for (unsigned int i = 0; i < NUM_CUBES; ++i) {
			GLuint result;
			glGetQueryObjectuiv(m_queries[i], GL_QUERY_RESULT, &result);
			if (result) m_rendered_cube_count++;
			glBeginConditionalRender(m_queries[i], GL_QUERY_NO_WAIT);
			RenderComplexScene(i);
			glEndConditionalRender();
		}
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Text("Rendered Cubes: %d", m_rendered_cube_count);
		ImGui::End();
	}
	void RenderOcclusionCube() {
		glEnable(GL_DEPTH_TEST);
		glUseProgram(m_occlusion_program);
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glUniform3fv(5, 1, glm::value_ptr(glm::vec3(0.0f, 2.0f, -8.0f)));
		glUniform3fv(6, 1, glm::value_ptr(glm::vec3(0.0f)));
		m_cube.OnDraw();
	}
	void RenderBasicScene(unsigned int n) {
		glEnable(GL_DEPTH_TEST);
		glUseProgram(m_program);
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glUniform1ui(5, n);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);
		m_cube.OnDraw();
	}
	void RenderComplexScene(unsigned int n) {
		RenderBasicScene(n);
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
#endif //OCCLUSION_QUERIES