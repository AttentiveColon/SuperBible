#include "Defines.h"
#ifdef SSAO
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


layout (location = 0) 
uniform mat4 model;
layout (location = 1)
uniform mat4 view;
layout (location = 2)
uniform mat4 proj;
layout (location = 3)
uniform vec3 cam_pos;
layout (location = 4)
uniform vec3 light_pos;


out VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;	
	vec2 uv;
} vs_out;



void main(void)
{
	vec4 P = model * vec4(position, 1.0);
	vs_out.N = mat3(model) * normal;
	vs_out.L = light_pos - P.xyz;
	vs_out.V = cam_pos - P.xyz;
	vs_out.uv = uv;

	gl_Position = proj * view * P;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D u_tex;

out vec4 color;

in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
} fs_in;

uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 300.0;
uniform vec3 ambient_power = vec3(0.01);

void main()
{

	vec3 diffuse_albedo = texture(u_tex, fs_in.uv).rgb;
	vec3 N = normalize(fs_in.N);
	vec3 L = normalize(fs_in.L);
	vec3 V = normalize(fs_in.V);
	vec3 R = reflect(-L, N);

	float NdotL = max(dot(N, L), 0.0);
	float RdotV = max(dot(R, V), 0.0);

	vec3 diffuse = NdotL * diffuse_albedo;
	vec3 specular = pow(RdotV, specular_power) * specular_albedo;
	vec3 ambient = diffuse_albedo * ambient_power;

	color = vec4(ambient + diffuse + specular, 1.0);
}
)";



static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	ObjMesh m_object;
	GLuint m_texture;

	glm::vec3 m_light_pos = glm::vec3(1.0f, 41.0f, 50.0f);

	SB::Camera m_camera;
	bool m_input_mode = false;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 8.0f, 15.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_program = LoadShaders(shader_text);
		m_object.Load_OBJ("./resources/monkey.obj");
		m_texture = Load_KTX("./resources/fiona.ktx");
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

		glm::mat4 model = glm::mat4(1.0);

		glUseProgram(m_program);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform3fv(3, 1, glm::value_ptr(m_camera.Eye()));
		glUniform3fv(4, 1, glm::value_ptr(m_light_pos));
		glBindTextureUnit(0, m_texture);
		m_object.OnDraw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat3("Light Pos", glm::value_ptr(m_light_pos), 0.1f);
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
#endif //SSAO