#include "Defines.h"
#ifdef TOON_SHADER
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* toon_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;


layout (location = 0) 
uniform mat4 projview;
layout(location = 1)
uniform mat4 model;


out VS_OUT
{
	vec3 FragPos;
	vec3 Normal;
} vs_out;



void main(void)
{
	vs_out.FragPos = vec3(model * vec4(position, 1.0));
	vs_out.Normal = transpose(inverse(mat3(model))) * normal;
	gl_Position = projview * vec4(vs_out.FragPos, 1.0);
}
)";

static const GLchar* toon_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler1D u_tex;

layout (location = 3)
uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);

out vec4 color;

in VS_OUT
{
	vec3 FragPos;
	vec3 Normal;
} fs_in;

uniform vec3 diffuse_albedo = vec3(0.9, 0.8, 1.0);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 300.0;


void main()
{
	vec3 diffuse_color = diffuse_albedo;
	vec3 normal = normalize(fs_in.Normal);
	vec3 lightColor = vec3(1.0);
	vec3 ambient = 0.15 * lightColor;
	vec3 lightDir = normalize(light_pos - fs_in.FragPos);
	float tc = max(dot(lightDir, normal), 0.0);
	//vec3 diffuse = diff * lightColor;


	vec3 lighting = texture(u_tex, tc).rgb;
	color = vec4(lighting, 1.0);
}
)";



static ShaderText toon_shader_text[] = {
	{GL_VERTEX_SHADER, toon_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, toon_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_toon_program;

	ObjMesh m_cube;

	GLuint m_toon_tex;

	glm::vec3 m_light_pos = glm::vec3(1.0f, 41.0f, 50.0f);

	SB::Camera m_camera;
	bool m_input_mode = false;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_toon_program = LoadShaders(toon_shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 8.0f, 15.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_cube.Load_OBJ("./resources/sphere.obj");

		glEnable(GL_DEPTH_TEST);

		static const GLubyte toon_tex_data[] = {
			0x44, 0x00, 0x00, 0x00,
			0x88, 0x00, 0x00, 0x00,
			0xcc, 0x00, 0x00, 0x00,
			0xff, 0x00, 0x00, 0x00,
		};

		glGenTextures(1, &m_toon_tex);
		glBindTexture(GL_TEXTURE_1D, m_toon_tex);
		glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGB8, sizeof(toon_tex_data) / 4);
		glTexSubImage1D(GL_TEXTURE_1D, 0, 0, sizeof(toon_tex_data) / 4, GL_RGBA, GL_UNSIGNED_BYTE, toon_tex_data);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
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

		ToonPassSetup();
		RenderScene();

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat3("Light Pos", glm::value_ptr(m_light_pos), 0.1f);
		ImGui::End();
	}
	void RenderScene() {
		glm::mat4 model = glm::rotate(sin((float)m_time), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::degrees(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(1.1f, 1.1f, 1.1f));
		glm::mat4 model2 = glm::translate(glm::vec3(0.0f, 5.0f, 5.0f)) * glm::rotate(cos((float)m_time), glm::vec3(1.0f, 1.0f, 0.0f)) * glm::rotate(glm::degrees(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model2));
		m_cube.OnDraw();
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
		m_cube.OnDraw();
	}
	void ToonPassSetup() {
		glUseProgram(m_toon_program);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glUniform3fv(3, 1, glm::value_ptr(m_light_pos));
		glBindTextureUnit(0, m_toon_tex);
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
#endif //TOON_SHADER