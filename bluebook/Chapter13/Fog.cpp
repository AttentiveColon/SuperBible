#include "Defines.h"
#ifdef FOG
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
layout (location = 2)
in vec2 uv;


layout (location = 0) 
uniform mat4 projview;
layout(location = 1)
uniform mat4 model;
layout (location = 2)
uniform mat4 u_view;


out VS_OUT
{
	vec3 FragPos;
	vec3 Normal;
	vec2 uv;
	vec3 EyePos;	
} vs_out;



void main(void)
{

	vs_out.FragPos = vec3(model * vec4(position, 1.0));
	vs_out.Normal = transpose(inverse(mat3(model))) * normal;
	vs_out.uv = uv;

	vs_out.EyePos = mat3(u_view) * vs_out.FragPos;
	gl_Position = projview * vec4(vs_out.FragPos, 1.0);
}
)";

static const GLchar* toon_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler1D u_tex;
layout (binding = 1)
uniform sampler2D u_texture;

layout (location = 3)
uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);

out vec4 color;

in VS_OUT
{
	vec3 FragPos;
	vec3 Normal;
	vec2 uv;
	vec3 EyePos;
} fs_in;

uniform vec3 diffuse_albedo = vec3(0.9, 0.8, 1.0);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 300.0;

uniform vec4 fog_color = vec4(0.7, 0.8, 0.9, 0.0);


layout (location = 10)
uniform bool fog_on = true;

vec4 fog(vec4 c)
{
	float z = length(fs_in.EyePos);
	float de = 0.25 * smoothstep(0.0, 6.0, 10.0 - fs_in.FragPos.y);
	float di = 0.45 * smoothstep(0.0, 40.0, 20.0 - fs_in.FragPos.y);
	float extinction = exp(-z * de);
	float inscattering = exp(-z * di);
	return c * extinction + fog_color * (1.0 - inscattering);
}

void main()
{

	vec3 diffuse_color = texture(u_texture, fs_in.uv).rgb;
	vec3 normal = normalize(fs_in.Normal);
	vec3 lightColor = vec3(1.0);
	vec3 ambient = 0.15 * lightColor;
	vec3 lightDir = normalize(light_pos - fs_in.FragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff * lightColor;


	vec3 lighting = (ambient + diffuse) * diffuse_color;
	color = vec4(lighting, 1.0);

	if (fog_on)
		color = fog(color);
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

	GLuint m_texture;
	GLuint m_toon_tex;

	glm::vec3 m_light_pos = glm::vec3(1.0f, 41.0f, 50.0f);

	SB::Camera m_camera;
	bool m_input_mode = false;

	bool m_fog_on = true;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_toon_program = LoadShaders(toon_shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 8.0f, 15.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_cube.Load_OBJ("./resources/cube.obj");
		m_texture = Load_KTX("./resources/trees.ktx");

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
		ImGui::Checkbox("Fog active", &m_fog_on);
		ImGui::End();
	}
	void RenderScene() {
		glm::mat4 model = glm::rotate(sin((float)m_time), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::degrees(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(2.1f, 2.1f, 2.1f));
		glm::mat4 model2 = glm::translate(glm::vec3(0.0f, 5.0f, 5.0f)) * glm::rotate(cos((float)m_time), glm::vec3(1.0f, 1.0f, 0.0f)) * glm::rotate(glm::degrees(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model2));
		m_cube.OnDraw();
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
		m_cube.OnDraw();
	}
	void ToonPassSetup() {
		glUseProgram(m_toon_program);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glUniform3fv(3, 1, glm::value_ptr(m_light_pos));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniform1i(10, m_fog_on);
		glBindTextureUnit(0, m_toon_tex);
		glBindTextureUnit(1, m_texture);
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
#endif //FOG