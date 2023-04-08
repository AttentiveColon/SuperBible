#include "Defines.h"
#ifdef MODEL_BASE
#include "System.h"
#include "Model.h"
#include <string>
#include <vector>

static const GLchar* vertex_shader_source = R"(
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
uniform mat3 u_normal_matrix;
layout (location = 5)
uniform vec3 u_view_pos;

layout (binding = 0, std140)
uniform DefaultUniform
{
	mat4 u_view;
	mat4 u_projection;
	vec2 u_resolution;
	float u_time;
};

layout (binding = 1, std140)
uniform LightUniform
{
	vec3 u_light_pos;
	float u_ambient_strength;
	vec3 u_light_color;
	float u_specular_strength;
};

out vec3 vs_normal;
out vec2 vs_uv;
out vec4 vs_frag_pos;

void main() 
{
	mat4 viewProj = u_projection * u_view;
	gl_Position = viewProj * u_model * vec4(position, 1.0);
	vs_normal = u_normal_matrix * normal;
	vs_uv = uv;
	vs_frag_pos = vec4(u_model[3][0], u_model[3][1], u_model[3][2], 1.0) * vec4(position, 1.0);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

layout (binding = 0, std140)
uniform DefaultUniform
{
	mat4 u_view;
	mat4 u_projection;
	vec2 u_resolution;
	float u_time;
};

layout (binding = 1, std140)
uniform LightUniform
{
	vec3 u_light_pos;
	float u_ambient_strength;
	vec3 u_light_color;
	float u_specular_strength;
};

layout (location = 5) uniform vec3 u_view_pos;
layout (location = 7) uniform vec4 u_base_color_factor;
layout (location = 8) uniform float u_alpha_cutoff;

in vec3 vs_normal;
in vec2 vs_uv;
in vec4 vs_frag_pos;

layout (binding = 0)
uniform sampler2D u_texture;

layout (binding = 2)
uniform sampler2D u_normal_texture;

out vec4 color;

void main() 
{
	//Get diffuse color
	vec4 diffuseColor = texture(u_texture, vs_uv) * u_base_color_factor;

	//Get normal map color
	vec3 perturbedNormal = normalize(texture2D(u_normal_texture, vs_uv).rgb * 2.0 - 1.0);
	vec3 surfaceNormal = normalize(perturbedNormal + vs_normal);

	//Calculate the diffuse lighting
	vec3 lightDirection = normalize(u_light_pos - vs_frag_pos.xyz);
	float diffuse = max(0.0, dot(surfaceNormal, lightDirection));

	//Calculate Specular
	vec3 viewDirection = normalize(-vs_frag_pos.xyz);
	vec3 halfwayDirection = normalize(lightDirection + viewDirection);
	float specular = pow(max(0.0, dot(surfaceNormal, halfwayDirection)), 16.0);

	//Get final fragment color
	vec3 ambientLight = u_light_color * diffuseColor.rgb * u_ambient_strength;
	vec3 diffuseLight = u_light_color * diffuseColor.rgb * diffuse;
	vec3 specularLight = u_light_color * specular * u_specular_strength;
	vec3 finalColor = ambientLight + diffuseLight + specularLight;

	color = vec4(finalColor, diffuseColor.a);
	if (color.a < u_alpha_cutoff) {
		discard;
	}
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct DefaultUniformBlock {		//std140
	glm::mat4 u_view;				//offset 0
	glm::mat4 u_proj;				//offset 16
	glm::vec2 u_resolution;			//offset 32
	GLfloat u_time;					//offset 34
};

struct LightUniformBlock {			//std140
	glm::vec3 u_light_pos;			//offset 0	
	GLfloat u_ambient_strength;		//offset 12
	glm::vec3 u_light_color;		//offset 16
	GLfloat u_specular_strength;	//offset 28
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	f64 m_dt;
	bool m_input_mode_active = true;

	GLuint m_program;
	SB::Model m_model;
	glm::vec3 m_cam_pos;
	float m_cam_rotation;

	SB::Camera m_camera;

	GLuint m_ubo;
	GLbyte* m_ubo_data;

	GLuint m_light_ubo;
	GLbyte* m_light_ubo_data;

	glm::vec3 m_light_pos;


	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		m_cam_pos(glm::vec3(0.0f, 10.0, 12.0)),
		m_cam_rotation(0.0f),
		m_light_pos(glm::vec3(0.0f, 1.0f, 0.0f))
	{}

	//TODO: Functions to set and update light position, ambient level and light color uniforms

	//TODO: Fix camera switching crash when no cameras in scene

	//TODO: Fix input jumping on first mouse movement

	//TODO: Look into and try to limit excessive loading on startup
	// (might have to create script to create custom model format)

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		//audio.PlayOneShot("./resources/startup.mp3");
		m_program = LoadShaders(shader_text);
		m_model = SB::Model("./resources/ABeautifulGame.glb");
		//m_model = SB::Model("./resources/sponza.glb");
		//m_model = SB::Model("../gltf_examples/2.0/sponza/glTF/Sponza.gltf");
		//m_model = SB::Model("./resources/alpha_test.glb");

		if (m_model.m_camera.m_cameras.size()) {
			m_camera = m_model.m_camera.GetCamera(0);
		}
		else {
			m_camera = SB::Camera("Camera", glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.1, 100.0);
		}
		
		input.SetRawMouseMode(window.GetHandle(), true);

		glCreateBuffers(1, &m_ubo);
		m_ubo_data = new GLbyte[sizeof(DefaultUniformBlock)];

		glCreateBuffers(1, &m_light_ubo);
		m_light_ubo_data = new GLbyte[sizeof(LightUniformBlock)];
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
		m_dt = dt;

		if (input.Pressed(GLFW_KEY_SPACE)) {
			m_camera = m_model.m_camera.GetNextCamera();
		}

		if (m_input_mode_active) {
			m_camera.OnUpdate(input, 3.0f, 0.2f, dt);
		}

		//Implement Camera Movement Functions
		if (input.Pressed(GLFW_KEY_LEFT_CONTROL)) {
			m_input_mode_active = !m_input_mode_active;
			input.SetRawMouseMode(window.GetHandle(), m_input_mode_active);
		}

		DefaultUniformBlock ubo;
		ubo.u_view = m_camera.m_view;
		ubo.u_proj = m_camera.m_proj;
		ubo.u_resolution = glm::vec2((float)window.GetWindowDimensions().width, (float)window.GetWindowDimensions().height);
		ubo.u_time = (float)m_time;
		memcpy(m_ubo_data, &ubo, sizeof(DefaultUniformBlock));

		LightUniformBlock light_ubo;
		light_ubo.u_light_pos = m_light_pos;
		light_ubo.u_ambient_strength = 0.3f;
		light_ubo.u_light_color = glm::vec3(1.0f, 1.0f, 1.0f);
		light_ubo.u_specular_strength = 0.2f;
		memcpy(m_light_ubo_data, &light_ubo, sizeof(LightUniformBlock));
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(DefaultUniformBlock), m_ubo_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);

		glBindBuffer(GL_UNIFORM_BUFFER, m_light_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(LightUniformBlock), m_light_ubo_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_light_ubo);

		glUseProgram(m_program);
		glUniform3fv(5, 1, glm::value_ptr(m_camera.Eye()));
		m_model.OnDraw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", (int)m_fps);
		ImGui::Text("Time: %f", (double)m_time);
		ImGui::Text("Frame Time: %f", (double)m_dt);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::LabelText("Current Camera", "{%d}", m_model.m_current_camera);
		ImGui::LabelText("Cam Position", "x: %f y: %f z: %f", m_camera.Eye().x, m_camera.Eye().y, m_camera.Eye().z);
		ImGui::LabelText("Camera Yaw", "%f", m_camera.m_yaw);
		ImGui::LabelText("Camera Pitch", "%f", m_camera.m_pitch);
		ImGui::LabelText("Forward Vector", "x: %f y: %f z: %f", m_camera.m_forward_vector.x, m_camera.m_forward_vector.y, m_camera.m_forward_vector.z);
		ImGui::DragFloat3("Light Position", glm::value_ptr(m_light_pos), 0.1f, -2.0f, 2.0f);
		ImGui::End();
	}
};

SystemConf config = {
		1280,					//width
		720,					//height
		300,					//Position x
		200,					//Position y
		"Application",			//window title
		false,					//windowed fullscreen
		false,					//vsync
		30,						//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //LOAD_GLTF_MESH
