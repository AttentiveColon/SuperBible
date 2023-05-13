#include "Defines.h"
#ifdef SHADOW_MAPPING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

#define DEPTH_TEXTURE_SIZE      4096
#define FRUSTUM_DEPTH           1000

static const GLchar* light_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

layout (location = 0)
uniform mat4 mvp;


void main(void)
{
	gl_Position = mvp * vec4(position, 1.0);
}
)";

static const GLchar* light_fragment_shader_source = R"(
#version 450 core

//out vec4 color;

void main()
{
	//color = vec4(gl_FragCoord.z);
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
uniform mat4 mv_matrix;
layout (location = 1) 
uniform mat4 proj_matrix;
layout (location = 2)
uniform mat4 shadow_matrix;

out VS_OUT
{
	vec4 shadow_coord;
	vec3 N;
	vec3 L;
	vec3 V;
} vs_out;

layout (location = 3)
uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);

void main(void)
{
	vec4 P = mv_matrix * vec4(position, 1.0);
	
	vs_out.N = mat3(mv_matrix) * normal;
	vs_out.L = light_pos - P.xyz;
	vs_out.V = -P.xyz;
	vs_out.shadow_coord = shadow_matrix * vec4(position, 1.0);

	gl_Position = proj_matrix * P;
}
)";

static const GLchar* view_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2DShadow shadow_tex;

out vec4 color;

in VS_OUT
{
	vec4 shadow_coord;
	vec3 N;
	vec3 L;
	vec3 V;
} fs_in;

uniform vec3 diffuse_albedo = vec3(0.9, 0.8, 1.0);
uniform vec3 specular_albedo = vec3(0.7);
uniform float specular_power = 300.0;
uniform bool full_shading = true;

void main()
{
	vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);

	vec3 R = reflect(-L, N);

	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
	vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_albedo;

	color = textureProj(shadow_tex, fs_in.shadow_coord) * vec4(diffuse + specular, 1.0);
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

		glGenFramebuffers(1, &m_shadow_buffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_buffer);		
		
		glGenTextures(1, &m_shadow_tex);
		glBindTexture(GL_TEXTURE_2D, m_shadow_tex);
		glTexStorage2D(GL_TEXTURE_2D, 11, GL_DEPTH_COMPONENT32F, DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadow_tex, 0);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glEnable(GL_DEPTH_TEST);

		RenderScene(true);

		RenderScene(false);

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat3("Light Pos", glm::value_ptr(m_light_pos), 0.1f);
		ImGui::End();
	}
	void RenderScene(bool from_light) {
		static const GLfloat one = 1.0f;
		static const glm::mat4 scale_bias_matrix = glm::mat4(glm::vec4(0.5f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.5f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.5f, 0.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));

		glm::mat4 light_model_matrix = glm::translate(m_light_pos);
		glm::mat4 light_view_matrix = glm::lookAt(m_light_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 light_proj_matrix = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1000.0f);
		glm::mat4 light_mvp_matrix = light_proj_matrix * light_view_matrix * light_model_matrix;
		glm::mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;
		glm::mat4 shadow_sbpv_matrix = scale_bias_matrix * light_proj_matrix * light_view_matrix;
		glm::mat4 model = glm::rotate(sin((float)m_time), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::degrees(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(1.1f, 1.1f, 1.1f));
		glm::mat4 model2 = glm::translate(glm::vec3(0.0f, 5.0f, 5.0f)) * glm::rotate(cos((float)m_time), glm::vec3(1.0f, 1.0f, 0.0f)) * glm::rotate(glm::degrees(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
		glm::mat4 shadow_matrix = shadow_sbpv_matrix * model;


		if (from_light) {
			glUseProgram(m_light_program);
			glBindTexture(GL_TEXTURE_2D, 0);
			glViewport(0, 0, DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
			glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_buffer);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(4.0f, 4.0f);
			static const GLenum buffers[] = { GL_NONE };
			glDrawBuffers(1, buffers);
			glClearBufferfv(GL_COLOR, 0, m_clear_color);
		}
		else {
			glClearBufferfv(GL_COLOR, 0, m_clear_color);
			glViewport(0, 0, 1600, 900);
			glUseProgram(m_view_program);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_shadow_tex);
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
			glDrawBuffer(GL_BACK);
		}

		//glClearBufferfv(GL_DEPTH, 0, &one);


		if (from_light) {
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(light_vp_matrix * model));
			m_cube.OnDraw();
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(light_vp_matrix * model2));
			m_cube.OnDraw();

			glDisable(GL_POLYGON_OFFSET_FILL);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		else {
			glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(shadow_matrix));
			glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
			glUniform3fv(3, 1, glm::value_ptr(m_light_pos));
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_camera.m_view * model));
			m_cube.OnDraw();
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_camera.m_view * model2));
			m_cube.OnDraw();
			glBindTexture(GL_TEXTURE_2D, 0);
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
#endif //SHADOW_MAPPING