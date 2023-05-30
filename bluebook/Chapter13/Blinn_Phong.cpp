#include "Defines.h"
#ifdef BLINN_PHONG
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* blinn_phong_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 3)
uniform mat4 u_model;
layout (location = 4)
uniform mat4 u_view;
layout (location = 5)
uniform mat4 u_proj;
layout (location = 6)
uniform vec3 u_cam_pos;

layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient;
};

out VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
} vs_out;

void main(void)
{
	

    vec4 P = u_model * vec4(position, 1.0);
	vs_out.N = mat3(u_model) * normal;
	vs_out.L = light_pos - P.xyz;
	vs_out.V = u_cam_pos - P.xyz;

	gl_Position = u_proj * u_view * P;
}
)";

static const GLchar* blinn_phong_fragment_shader_source = R"(
#version 450 core

layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient;
};

in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
} fs_in;

out vec4 color;

void main()
{
	vec3 N = normalize(fs_in.N);
	vec3 L = normalize(fs_in.L);
	vec3 V = normalize(fs_in.V);

	vec3 H = normalize(L + V);

	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
	vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;

	color = vec4(ambient + diffuse + specular, 1.0);
}
)";

static ShaderText blinn_phong_shader_text[] = {
	{GL_VERTEX_SHADER, blinn_phong_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, blinn_phong_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* phong_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 3)
uniform mat4 u_model;
layout (location = 4)
uniform mat4 u_view;
layout (location = 5)
uniform mat4 u_proj;
layout (location = 6)
uniform vec3 u_cam_pos;

layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient;
};

out VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
} vs_out;

void main(void)
{

    vec4 P = u_model * vec4(position, 1.0);
	vs_out.N = mat3(u_model) * normal;
	vs_out.L = light_pos - P.xyz;
	vs_out.V = u_cam_pos - P.xyz;

	gl_Position = u_proj * u_view * P;
}
)";

static const GLchar* phong_fragment_shader_source = R"(
#version 450 core

layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient;
};

in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
} fs_in;

out vec4 color;

void main()
{
	vec3 N = normalize(fs_in.N);
	vec3 L = normalize(fs_in.L);
	vec3 V = normalize(fs_in.V);

	vec3 R = reflect(-L, N);

	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
	vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_albedo;

	color = vec4(ambient + diffuse + specular, 1.0);
}
)";

static ShaderText phong_shader_text[] = {
	{GL_VERTEX_SHADER, phong_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, phong_fragment_shader_source, NULL},
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
};


struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_blinn_phong_program, m_phong_program;

	GLuint m_ubo;
	Material_Uniform* m_data;

	glm::vec3 m_light_pos = glm::vec3(100.0f, 100.0f, 100.0f);
	glm::vec3 m_diffuse_albedo = glm::vec3(0.5f, 0.2f, 0.7f);
	glm::vec3 m_specular_albedo = glm::vec3(0.7f);
	float m_specular_power = 128.0f;
	glm::vec3 m_ambient = vec3(0.1f, 0.1f, 0.1f);


	ObjMesh m_cube;
	glm::mat4 m_cube_rotation;

	SB::Camera m_camera;
	bool m_input_mode = false;

	bool m_use_phong = false;

	Random m_random;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_blinn_phong_program = LoadShaders(blinn_phong_shader_text);
		m_phong_program = LoadShaders(phong_shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 4.0f, 2.0f), glm::vec3(0.0f, 5.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_cube.Load_OBJ("./resources/Skull/Skull.obj");
		//m_cube.Load_OBJ("./resources/smooth_sphere.obj");
		m_random.Init();

		glGenBuffers(1, &m_ubo);
		m_data = new Material_Uniform;
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

		m_cube_rotation = glm::rotate(-90.0f, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(cos((float)m_time * 0.1f), glm::vec3(0.0, 0.0, 1.0)) * glm::scale(glm::vec3(0.1f, 0.1f, 0.1f));

		Material_Uniform temp_material = { m_light_pos, 0.0, m_diffuse_albedo, 0.0, m_specular_albedo, m_specular_power, m_ambient };
		memcpy(m_data, &temp_material, sizeof(Material_Uniform));
	}
	void OnDraw() {
		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glEnable(GL_DEPTH_TEST);
		if (m_use_phong)
			glUseProgram(m_phong_program);
		else
			glUseProgram(m_blinn_phong_program);
		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(m_cube_rotation));
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform3fv(6, 1, glm::value_ptr(m_camera.Eye()));

		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(Material_Uniform), m_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);

		m_cube.OnDraw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat3("Light Position", glm::value_ptr(m_light_pos), 0.1f);
		ImGui::ColorEdit3("Diffuse Albedo", glm::value_ptr(m_diffuse_albedo));
		ImGui::ColorEdit3("Specular Albedo", glm::value_ptr(m_specular_albedo));
		ImGui::DragFloat("Specular Power", &m_specular_power, 0.1f);
		ImGui::ColorEdit3("Ambient", glm::value_ptr(m_ambient));
		ImGui::Checkbox("Use Phong Shader", &m_use_phong);
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
#endif //BLINN_PHONG