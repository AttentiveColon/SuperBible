#include "Defines.h"
#ifdef SHADOWMAP2
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

#define DEPTH_TEXTURE_SIZE      2048
#define FRUSTUM_DEPTH           1000

static const GLchar* light_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

layout (location = 0)
uniform mat4 viewproj;
layout (location = 1)
uniform mat4 model;


void main(void)
{
	gl_Position = viewproj * model * vec4(position, 1.0);
}
)";

static const GLchar* light_fragment_shader_source = R"(
#version 450 core


void main()
{

}
)";



static ShaderText light_shader_text[] = {
	{GL_VERTEX_SHADER, light_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, light_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* view_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;


layout (location = 0) 
uniform mat4 projview;
layout(location = 1)
uniform mat4 model;
layout (location = 2)
uniform mat4 light_viewproj;

out VS_OUT
{
	vec3 FragPos;
	vec3 Normal;
	vec4 FragPosLightSpace;
} vs_out;



void main(void)
{
	vs_out.FragPos = vec3(model * vec4(position, 1.0));
	vs_out.Normal = transpose(inverse(mat3(model))) * normal;
	vs_out.FragPosLightSpace = light_viewproj * vec4(vs_out.FragPos, 1.0);
	gl_Position = projview * vec4(vs_out.FragPos, 1.0);
}
)";

static const GLchar* view_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D shadow_tex;

layout (location = 3)
uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);

out vec4 color;

in VS_OUT
{
	vec3 FragPos;
	vec3 Normal;
	vec4 FragPosLightSpace;
} fs_in;

uniform vec3 diffuse_albedo = vec3(0.9, 0.8, 1.0);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 300.0;


float ShadowCalculation(vec4 shadow_coord)
{
	vec3 projCoords = shadow_coord.xyz / shadow_coord.w;
	projCoords = projCoords * 0.5 + 0.5;
	float closestDepth = texture(shadow_tex, projCoords.xy).r;
	float currentDepth = projCoords.z;
	float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
	return shadow;
}

void main()
{
	vec3 diffuse_color = diffuse_albedo;
	vec3 normal = normalize(fs_in.Normal);
	vec3 lightColor = vec3(1.0);
	vec3 ambient = 0.15 * lightColor;
	vec3 lightDir = normalize(light_pos - fs_in.FragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff * lightColor;
	float shadow = ShadowCalculation(fs_in.FragPosLightSpace);


	vec3 lighting = (ambient + (1.0 - shadow) * diffuse) * diffuse_color;
	color = vec4(lighting, 1.0);
}
)";



static ShaderText view_shader_text[] = {
	{GL_VERTEX_SHADER, view_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, view_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_light_program, m_view_program;

	ObjMesh m_cube;

	GLuint m_shadow_buffer, m_shadow_tex;

	glm::vec3 m_light_pos = glm::vec3(1.0f, 41.0f, 50.0f);

	SB::Camera m_camera;
	bool m_input_mode = false;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_light_program = LoadShaders(light_shader_text);
		m_view_program = LoadShaders(view_shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 8.0f, 15.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_cube.Load_OBJ("./resources/cube.obj");

		glEnable(GL_DEPTH_TEST);

		glGenFramebuffers(1, &m_shadow_buffer);

		glGenTextures(1, &m_shadow_tex);
		glBindTexture(GL_TEXTURE_2D, m_shadow_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_buffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadow_tex, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

		glViewport(0, 0, DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_buffer);
		glClear(GL_DEPTH_BUFFER_BIT);

		//ConfigureShaderAndMatrices()
		LightPassSetup();
		RenderScene();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//ConfigureShaderAndMatrices()
		ViewPassSetup();
		glBindTexture(GL_TEXTURE_2D, m_shadow_tex);
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
	void LightPassSetup() {
		glm::mat4 light_model_matrix = glm::translate(m_light_pos);
		glm::mat4 light_view_matrix = glm::lookAt(m_light_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 light_proj_matrix = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1000.0f);
		glm::mat4 light_vp_matrix = light_proj_matrix * light_view_matrix * light_model_matrix;

		glUseProgram(m_light_program);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(light_vp_matrix));
	}
	void RenderScene() {
		glm::mat4 model = glm::rotate(sin((float)m_time), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::degrees(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(1.1f, 1.1f, 1.1f));
		glm::mat4 model2 = glm::translate(glm::vec3(0.0f, 5.0f, 5.0f)) * glm::rotate(cos((float)m_time), glm::vec3(1.0f, 1.0f, 0.0f)) * glm::rotate(glm::degrees(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model2));
		m_cube.OnDraw();
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
		m_cube.OnDraw();
	}
	void ViewPassSetup() {
		glm::mat4 light_model_matrix = glm::translate(m_light_pos);
		glm::mat4 light_view_matrix = glm::lookAt(m_light_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 light_proj_matrix = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1000.0f);
		glm::mat4 light_vp_matrix = light_proj_matrix * light_view_matrix * light_model_matrix;

		glUseProgram(m_view_program);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(light_vp_matrix));
		glUniform3fv(3, 1, glm::value_ptr(m_light_pos));
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
#endif //SHADOWMAP2