#include "Defines.h"
#ifdef ENVIROMENT_MAPPING_REDUX
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* skybox_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

out VS_OUT
{
    vec3    tc;
} vs_out;

layout (location = 1)
uniform mat4 u_view;
layout (location = 2)
uniform mat4 u_proj;

void main(void)
{
	vec4 pos = u_proj * u_view * vec4(position, 1.0);
	gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
	vs_out.tc = vec3(position.x, position.y, -position.z);
}

)";

static const GLchar* skybox_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform samplerCube tex_cubemap;

in VS_OUT
{
    vec3    tc;
} fs_in;

layout (location = 0) out vec4 color;

void main(void)
{
    color = texture(tex_cubemap, fs_in.tc);
}
)";

static ShaderText skybox_shader_text[] = {
	{GL_VERTEX_SHADER, skybox_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, skybox_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;

out VS_OUT
{
    vec3 N;
	vec3 L;
	vec3 V;
	vec3 P;
} vs_out;

layout (location = 0)
uniform mat4 u_model;
layout (location = 1)
uniform mat4 u_view;
layout (location = 2)
uniform mat4 u_proj;
layout (location = 3)
uniform vec3 u_eye;

uniform vec3 light_pos = vec3(0.0, 100.0, 100.0);

void main(void)
{
	gl_Position = u_proj * u_view * u_model * vec4(position, 1.0);
	vs_out.N = mat3(u_model) * normal;
	vs_out.P = mat3(u_model) * position;
	vs_out.L = light_pos - vs_out.P;
	vs_out.V = u_eye - vs_out.P;
}

)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform samplerCube tex_cubemap;

in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec3 P;
} fs_in;

layout (location = 0) out vec4 color;

layout (location = 5) uniform bool is_reflect;

uniform vec3 diffuse_albedo = vec3(0.8, 0.8, 0.8);
uniform float specular_power = 256.0;
uniform float refraction_index = 0.1;
uniform float reflect_factor = 0.4;

void main(void)
{
	vec3 N = normalize(fs_in.N);
	vec3 L = normalize(fs_in.L);
	vec3 V = normalize(fs_in.V);
	

	
	vec3 R = reflect(-L, N);


	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
	vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * vec3(1.0);

	vec3 tc = refract(normalize(-fs_in.V), normalize(fs_in.N), refraction_index); 
	if (is_reflect)
		tc = reflect(-fs_in.P, normalize(fs_in.N));
    vec3 reflectance = texture(tex_cubemap, tc).rgb;

	color = vec4((specular) * reflect_factor + reflectance, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static void Draw(const Mesh& mesh) {
	glBindVertexArray(mesh.vao);
	glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, (void*)0);
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_skybox_program, m_program;

	Mesh m_cube;
	GLuint m_cube_map;

	Mesh m_sphere;


	SB::Camera m_camera;
	bool m_input_mode = false;

	bool m_reflect = true;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_skybox_program = LoadShaders(skybox_shader_text);
		m_program = LoadShaders(shader_text);

		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 2000.0);

		m_cube = ImportMesh("./resources/cube.obj");
		m_cube_map = Load_KTX("./resources/mountaincube.ktx");
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_cube_map);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		m_sphere = ImportMesh("./resources/smooth_sphere.obj");
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

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glUseProgram(m_skybox_program);
		glBindTextureUnit(0, m_cube_map);
		//Cast to mat3 then back to mat4 so last row is zero, thus having no effect on translations, then you will always be at the center of the skybox
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(m_camera.m_view))));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		Draw(m_cube);

		glUseProgram(m_program);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform1i(5, m_reflect);
		glUniform3fv(3, 1, glm::value_ptr(m_camera.Eye()));
		Draw(m_sphere);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Reflect", &m_reflect);
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
#endif //ENVIROMENT_MAPPING_REDUX