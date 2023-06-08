#include "Defines.h"
#ifdef GEOMETRY_EXPLOSION_SHADER
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec2 uv;

out VS_OUT
{
	vec2 uv;
} vs_out;

void main()
{
	gl_Position = vec4(position, 1.0);
	vs_out.uv = uv;
}
)";

static const GLchar* geometry_shader_source = R"(
#version 450 core

layout (triangles) in;
layout (triangle_strip) out;
layout (max_vertices = 3) out;

layout (location = 12)
uniform mat4 mvp;

layout (location = 13)
uniform float explosion_factor;

in VS_OUT
{
	vec2 uv;
} gs_in[];

out GS_OUT
{
	vec2 uv;
} gs_out;

void main()
{
	vec3 ab = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 ac = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 normal = -normalize(cross(ab, ac));

	for (int i = 0; i < gl_in.length(); ++i)
	{
		gl_Position = mvp * (gl_in[i].gl_Position + vec4(explosion_factor * normal, 0.0));
		gs_out.uv = gs_in[i].uv;
		EmitVertex();
	}
	EndPrimitive();
	
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

in GS_OUT
{
	vec2 uv;
} fs_in;

void main()
{
	color = vec4(fs_in.uv.x, fs_in.uv.y, 1.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_GEOMETRY_SHADER, geometry_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLfloat buffer[] = {
		1.000000, 1.000000, -1.000000,  0.625000, 0.500000,
		1.000000, 1.000000, -1.000000,  0.625000, 0.500000,
		1.000000, 1.000000, -1.000000,  0.625000, 0.500000,
		1.000000, -1.000000, -1.000000, -0.375000, 0.500000,
		1.000000, -1.000000, -1.000000,  0.375000, 0.500000,
		1.000000, -1.000000, -1.000000,  0.375000, 0.500000,
		1.000000, 1.000000, 1.000000, 0.625000, 0.250000,
		1.000000, 1.000000, 1.000000,  0.625000, 0.250000,
		1.000000, 1.000000, 1.000000,  0.625000, 0.250000,
		1.000000, -1.000000, 1.000000,  -0.375000, 0.250000,
		1.000000, -1.000000, 1.000000, 0.375000, 0.250000,
		1.000000, -1.000000, 1.000000,  0.375000, 0.250000,
		-1.000000, 1.000000, -1.000000,  0.625000, 0.750000,
		-1.000000, 1.000000, -1.000000, 0.625000, 0.750000,
		-1.000000, 1.000000, -1.000000, 0.875000, 0.500000,
		-1.000000, -1.000000, -1.000000, -0.125000, 0.500000,
		-1.000000, -1.000000, -1.000000,  0.375000, 0.750000,
		-1.000000, -1.000000, -1.000000, 0.375000, 0.750000,
		-1.000000, 1.000000, 1.000000, 0.625000, 0.000000,
		-1.000000, 1.000000, 1.000000,  0.625000, 1.000000,
		-1.000000, 1.000000, 1.000000,  0.875000, 0.250000,
		-1.000000, -1.000000, 1.000000, -0.125000, 0.250000,
		-1.000000, -1.000000, 1.000000, 0.375000, 0.000000,
		-1.000000, -1.000000, 1.000000,  0.375000, 1.000000
};

static const GLint buffer_indices[] = {
		1, 14, 20,
		1, 20, 7,
		10, 6, 18,
		10, 18, 22,
		23, 19, 12,
		23, 12, 16,
		15, 3, 9,
		15, 9, 21,
		5, 2, 8,
		5, 8, 11,
		17, 13, 0,
		17, 0, 4
};


struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;


	GLuint m_vao;

	float m_explosion_factor = 1.0f;

	SB::Camera m_camera;
	bool m_input_mode = false;


	bool m_wireframe = false;
	ObjMesh m_mesh;


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

		//SB::ModelDump("./resources/cube.glb");
		m_mesh.Load_OBJ("./resources/monkey.obj");

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, NULL);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(sizeof(float) * 3));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		GLuint ebo;
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(buffer_indices), buffer_indices, GL_STATIC_DRAW);

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);
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
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);
		
		

		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glUniformMatrix4fv(12, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glUniform1f(13, m_explosion_factor);

		m_mesh.OnDraw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
		ImGui::SliderFloat("ExplosionFactor", &m_explosion_factor, 0.0f, 15.0f);
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
#endif //GEOMETRY_EXPLOSION_SHADER
