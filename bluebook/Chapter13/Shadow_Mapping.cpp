#include "Defines.h"
#ifdef SHADOW_MAPPING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* gloss_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;
layout (location = 3)
in vec3 tangent;

layout (location = 1)
uniform mat4 u_view;
layout (location = 2)
uniform mat4 u_proj;

out VS_OUT
{
	vec3 normal;
	vec3 view;
	vec2 uv;
} vs_out;

void main(void)
{
	vec4 P = u_view * vec4(position, 1.0);
	
	vs_out.normal = mat3(u_view) * normal;
	vs_out.view = P.xyz;
	vs_out.uv = uv;

	gl_Position = u_proj * P;
}
)";

static const GLchar* gloss_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler3D u_tex_envmap;
layout (binding = 1)
uniform sampler2D u_tex_glossmap;

in VS_OUT
{
	vec3 normal;
	vec3 view;
	vec2 uv;
} fs_in;

out vec4 color;

void main()
{
	vec3 u = normalize(fs_in.view);
	vec3 r = reflect(u, normalize(fs_in.normal));
	
	r.z += 1.0;
	float m = 0.5 * inversesqrt(dot(r, r));

	float gloss = texture(u_tex_glossmap, fs_in.uv * vec2(3.0, 1.0) * 2.0).r;
	vec3 env_coord = vec3(r.xy * m + vec2(0.5), gloss);
	color = texture(u_tex_envmap, env_coord);
}
)";



static ShaderText gloss_shader_text[] = {
	{GL_VERTEX_SHADER, gloss_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, gloss_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Material_Uniform {
	glm::vec3 light_pos;
	float pad0;
	glm::vec3 diffuse_albedo;
	float pad1;
	glm::vec3 specular_albedo;
	float specular_power;
	glm::vec3 ambient;
	float pad2;
	glm::vec3 rim_color;
	float rim_power;
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;

	ObjMesh m_cube;

	GLuint m_env_map;
	GLuint m_gloss_map;


	SB::Camera m_camera;
	bool m_input_mode = false;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(gloss_shader_text);


		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);

		m_cube.Load_OBJ("./resources/cube.obj");
		m_env_map = Load_KTX("./resources/mountains3d.ktx");
		m_gloss_map = Load_KTX("./resources/pattern1.ktx");
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

		glEnable(GL_DEPTH_TEST);
		glUseProgram(m_program);

		glBindTextureUnit(0, m_env_map);
		glBindTextureUnit(1, m_gloss_map);

		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		m_cube.OnDraw();
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
#endif //SHADOW_MAPPING