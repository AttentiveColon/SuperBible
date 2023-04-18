#include "Defines.h"
#ifdef CHANGING_PRIMITIVE_TYPE
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

out VS_OUT
{
	vec3 normal;
	vec2 uv;
} vs_out;

void main()
{
	gl_Position = vec4(position, 1.0);
	vs_out.normal = normal;
	vs_out.uv = uv;
}
)";

static const GLchar* geometry_shader_source = R"(
#version 450 core

layout (triangles) in;
layout (line_strip) out;
layout (max_vertices = 8) out;

layout (location = 12)
uniform mat4 mvp;

layout (location = 13)
uniform float normal_length;

in VS_OUT
{
	vec3 normal;
	vec2 uv;
} gs_in[];

out GS_OUT
{
	vec3 normal;
	vec2 uv;
} gs_out;

void main()
{
	for (int i = 0; i < gl_in.length(); i++)
	{
		gl_Position = mvp * gl_in[i].gl_Position;
		gs_out.normal = gs_in[i].normal;
		gs_out.uv = gs_in[i].uv;
		EmitVertex();
		gl_Position = mvp * (gl_in[i].gl_Position + (vec4(gs_in[i].normal * normal_length, 0.0)));
		gs_out.normal = gs_in[i].normal;
		gs_out.uv = gs_in[i].uv;
		EmitVertex();
		EndPrimitive();
	}
	
	vec3 ab = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 ac = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 face_normal = normalize(cross(ab, ac));

	vec4 tri_centroid = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3.0;

	gl_Position = mvp * tri_centroid;
	gs_out.normal = gs_in[0].normal;
	gs_out.uv = gs_in[0].uv;
	EmitVertex();

	gl_Position = mvp * (tri_centroid + vec4(face_normal * normal_length, 0.0));
	gs_out.normal = gs_in[0].normal;
	gs_out.uv = gs_in[0].uv;
	EmitVertex();
	EndPrimitive();
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

in GS_OUT
{
	vec3 normal;
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

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;


	GLuint m_vao;

	float m_explosion_factor = 0.2f;

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

		m_mesh.Load_OBJ("./resources/monkey.obj");

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
		ImGui::SliderFloat("ExplosionFactor", &m_explosion_factor, 0.1f, 0.9f);
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
#endif //CHANGING_PRIMITIVE_TYPE
