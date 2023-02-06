#include "Defines.h"
#ifdef UNIFORM_BUFFER
#include "System.h"

#include "glm/common.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (binding = 0) uniform DefaultUniform
{
	mat4 u_model;
	mat4 u_viewproj;
	vec2 u_resolution;
	float u_time;
};

out vec4 vs_normal;
out vec2 vs_uv;

void main() 
{
	vec4 new_pos = u_viewproj * u_model * vec4(position.x, position.y, position.z, 1.0);
	gl_Position = new_pos;
	vs_normal = u_model * vec4(normal, 1.0);
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in vec4 vs_normal;
in vec2 vs_uv;

out vec4 color;

void main() 
{
	color = vec4(0.5 * (vs_normal.y * vs_normal.x + 0.1) + 0.1, 0.5 * (vs_normal.y * (vs_normal.z + 0.2)) + 0.1, 0.5 * (vs_normal.y * (vs_normal.x + 0.3)) + 0.1, 1.0);
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
	WindowXY m_resolution;

	ObjMesh m_mesh;
	GLuint m_program;
	GLuint m_ubo;

	GLubyte* m_block_buffer;
	GLint m_block_size;
	GLuint m_indices[4];
	GLint m_offsets[4];

	glm::vec3 m_cam_pos;
	glm::vec3 m_direction;
	GLfloat m_angle_x;
	GLfloat m_angle_y;

	glm::mat4 m_viewproj;
	glm::mat4 m_model;

	GLfloat m_model_rot_z;
	GLfloat m_model_rot_x;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0),
		m_cam_pos(4.0f, 4.0f, -6.0f),
		m_direction(0.0f, 0.0f, 0.0f),
		m_angle_x(0.0),
		m_angle_y(0.0),
		m_model_rot_z(10.0f),
		m_model_rot_x(0.0f)
	{
		m_direction = glm::normalize(m_direction - m_cam_pos);
	}

	void OnInit(Input& input, Audio& audio, Window& window) {
		input.SetRawMouseMode(window.GetHandle(), true);
		audio.PlayOneShot("./resources/startup.mp3");
		glEnable(GL_DEPTH_TEST);

		m_program = LoadShaders(shader_text);
		GLuint blockIndex = glGetUniformBlockIndex(m_program, "DefaultUniform");
		
		glGetActiveUniformBlockiv(m_program, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &m_block_size);

		m_block_buffer = (GLubyte*)malloc(m_block_size);

		const GLchar* names[] = { "u_model", "u_viewproj", "u_resolution", "u_time" };
		
		glGetUniformIndices(m_program, 4, names, m_indices);

		glGetActiveUniformsiv(m_program, 4, m_indices, GL_UNIFORM_OFFSET, m_offsets);

		glGenBuffers(1, &m_ubo);


		m_mesh.Load_OBJ("./resources/basic_scene.obj");
		m_resolution = window.GetWindowDimensions();

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (input.Pressed(GLFW_KEY_BACKSPACE)) {
			if (input.IsMouseRawActive()) {
				input.SetRawMouseMode(window.GetHandle(), false);
			}
			else {
				input.SetRawMouseMode(window.GetHandle(), true);
			}
		}

		if (input.IsMouseRawActive()) {
			MousePos mouse_pos = input.GetMouseRaw();
			m_angle_x += mouse_pos.x * 0.1;
			m_angle_y = glm::clamp(m_angle_y + (mouse_pos.y * 0.1), -45.0, 45.0);
		}

		glm::vec3 front;
		front.x = cos(glm::radians(m_angle_x) * cos(glm::radians(0.0)));
		front.y = -sin(glm::radians(m_angle_y));
		front.z = sin(glm::radians(m_angle_x) * cos(glm::radians(0.0)));
		m_direction = front + m_cam_pos;


		if (input.Held(GLFW_KEY_W)) {
			m_cam_pos += glm::normalize(front) * (f32)dt * 2.0f;
		}
		if (input.Held(GLFW_KEY_S)) {
			m_cam_pos += -glm::normalize(front) * (f32)dt * 2.0f;
		}

		glm::mat4 model = glm::mat4(1.0f);
		m_model = glm::rotate(model, glm::radians(m_model_rot_z * (float)sin(m_time)), glm::vec3(0.0f, sin(m_time) * 50.0f, 1.0f));
		m_model = glm::rotate(m_model, glm::radians(m_model_rot_x), glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 lookat = glm::lookAt(m_cam_pos, m_direction, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 perspective = glm::perspective(90.0f, 16.0f / 9.0f, 0.1f, 10000.0f);
		m_viewproj = perspective * lookat;

		memcpy(m_block_buffer + m_offsets[0], &m_model, sizeof(glm::mat4));
		memcpy(m_block_buffer + m_offsets[1], &m_viewproj, sizeof(glm::mat4));
		memcpy(m_block_buffer + m_offsets[2], &m_resolution, sizeof(glm::vec2));
		memcpy(m_block_buffer + m_offsets[3], &m_time, sizeof(GLfloat));

		m_mesh.OnUpdate(dt);
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program);
		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, m_block_size, m_block_buffer, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);

		m_mesh.OnDraw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat3("Cam Pos", glm::value_ptr(m_cam_pos));
		ImGui::DragFloat("Model Rotation Z", &m_model_rot_z, 1.0f, 0.0f, 90.0f);
		ImGui::DragFloat("Model Rotation X", &m_model_rot_x, 1.0f, 0.0f, 90.0f);
		ImGui::Text("AngleX: %f", m_angle_x);
		ImGui::Text("AngleY: %f", m_angle_y);
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
#endif //TEST
