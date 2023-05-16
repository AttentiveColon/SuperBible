#include "Defines.h"
#ifdef DEFERRED_SHADING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* phong_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 0) 
uniform mat4 model;
layout (location = 1)
uniform mat4 view;
layout (location = 2)
uniform mat4 proj;


layout (location = 10)
uniform vec3 light_position = vec3(100.0, 100.0, 100.0);

out VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
} vs_out;

void main()
{	
	mat4 mv_matrix = view * model;
	vec4 P = mv_matrix * vec4(position, 1.0);
	vs_out.N = mat3(mv_matrix) * normal;
	vs_out.L = light_position - P.xyz;
	vs_out.V = -P.xyz;
	vs_out.uv = uv;

	gl_Position = proj * P;
}

)";


static const GLchar* phong_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D diffuse_texture;

uniform vec3 specular_albedo = vec3(1.0);
uniform float specular_power = 128.0;

out vec4 color;

in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
} fs_in;

void main()
{
	vec3 diffuse_albedo = texture(diffuse_texture, fs_in.uv).rgb;
	
	vec3 N = normalize(fs_in.N);
	vec3 L = normalize(fs_in.L);
	vec3 V = normalize(fs_in.V);

	vec3 R = reflect(-L, N);

	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
	vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_albedo;

	color = vec4(diffuse + specular, 1.0);
}
)";

static ShaderText phong_shader_text[] = {
	{GL_VERTEX_SHADER, phong_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, phong_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_phong_program;
	ObjMesh m_cube;
	GLuint m_tex;

	glm::vec3 m_view_pos = glm::vec3(-10.0f);

	glm::vec3 m_light_pos = glm::vec3(1.0f, 41.0f, 50.0f);

	SB::Camera m_camera;
	bool m_input_mode = false;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_phong_program = LoadShaders(phong_shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 8.0f, 15.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_tex = Load_KTX("./resources/fiona.ktx");
		m_cube.Load_OBJ("./resources/sphere.obj");

		glEnable(GL_DEPTH_TEST);

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
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Render();

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat3("Light Pos", glm::value_ptr(m_light_pos), 0.1f);
		ImGui::DragFloat3("Camera Pos", glm::value_ptr(m_view_pos), 0.1f);
		ImGui::End();
	}
	void Render() {
		float time = float(m_time) * 1.1f;

		glm::vec3 light_pos = glm::vec3(sin(time) * 100.0f, cos(time) * 100.0f, 100.0f);

		glm::mat4 view_matrix = glm::lookAt(m_view_pos, glm::vec3(2.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj_matrix = glm::perspective(90.0f, 16.0f / 9.0f, 0.01f, 1000.0f);

		glUseProgram(m_phong_program);
		glBindTextureUnit(0, m_tex);
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_matrix));
		//glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		//glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));

		glUniform3fv(10, 1, glm::value_ptr(light_pos));

		float scaling = 2.0f;
		int size = 10;
		glm::mat4 model = glm::mat4(1.0f);
		for (int z = 0; z < size; ++z) 
			for (int y = 0; y < size; ++y)
				for (int x = 0; x < size; ++x) {
					model = glm::translate(glm::vec3(x * scaling, y * scaling, z * scaling)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
					glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
					m_cube.OnDraw();
				}		
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
#endif //DEFERRED_SHADING