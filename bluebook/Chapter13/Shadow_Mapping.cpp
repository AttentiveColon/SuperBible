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

layout (location = 0)
uniform mat4 u_model = mat4(1.0);
layout (location = 1)
uniform mat4 u_view;
layout (location = 2)
uniform mat4 u_proj;
layout (location = 3)
uniform vec3 u_light_pos;

out VS_OUT
{
	vec3 normal;
	vec3 light;
	vec3 view;
	vec2 uv;
} vs_out;

void main(void)
{
	mat4 mv_matrix = u_view * u_model;

	vec4 P = mv_matrix * vec4(position, 1.0);	
	vs_out.normal = mat3(mv_matrix) * normal;
	vs_out.light = u_light_pos - P.xyz;
	vs_out.view = -P.xyz;
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

uniform vec3 ambient = vec3(0.1);

in VS_OUT
{
	vec3 normal;
	vec3 light;
	vec3 view;
	vec2 uv;
} fs_in;

out vec4 color;

void main()
{
	vec3 N = normalize(fs_in.normal);
	vec3 L = normalize(fs_in.light);
	vec3 V = normalize(fs_in.view);

	vec3 R = reflect(-L, N);

	vec3 diffuse_color = texture(u_tex_glossmap, fs_in.uv).rgb;
	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_color;

	vec3 specular = pow(max(dot(R, V), 0.0), 128.0) * vec3(1.0);

	color = vec4(ambient + diffuse + specular, 1.0);
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

		m_cube.Load_OBJ("./resources/sphere.obj");
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

		glm::mat4 model = glm::rotate(sin((float)m_time), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform3fv(3, 1, glm::value_ptr(glm::vec3(15.0f, 5.0f, 50.0f)));
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