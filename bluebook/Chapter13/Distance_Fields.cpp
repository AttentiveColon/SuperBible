#include "Defines.h"
#ifdef DISTANCE_FIELDS
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* vertex_shader_source = R"(
#version 430 core

out vec3 uv;

layout (location = 0) uniform mat3 uv_transform;
layout (location = 1) uniform mat4 mvp_matrix;

void main(void)
{
    const vec2 pos[] = vec2[](vec2(-1.0, -1.0),
                              vec2(1.0, -1.0),
                              vec2(-1.0, 1.0),
                              vec2(1.0, 1.0));

    vec3 uv3 = uv_transform * vec3( (pos[gl_VertexID] + vec2(1.0, 1.0)) * vec2(0.5, -0.5), 1.0);
    uv.xy = uv3.xy / uv3.z;
    uv.z = float(gl_InstanceID);
    gl_Position = mvp_matrix * vec4(pos[gl_VertexID] + vec2(gl_InstanceID * 2, 0.0), 0.0, 1.0);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 430 core

layout (location = 0) out vec4 color;

layout (binding = 0) uniform sampler2D sdf_texture;

in vec3 uv;

void main(void)
{
    const vec4 c1 = vec4(0.1, 0.1, 0.2, 1.0);
    const vec4 c2 = vec4(0.8, 0.9, 1.0, 1.0);
    float val = texture(sdf_texture, vec2(uv.xy)).x;

    color = mix(c1, c2, smoothstep(0.52, 0.48, val));
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
	GLuint m_logo_texture;

	SB::Camera m_camera;
	bool m_input_mode = false;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 8.0f, 15.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_logo_texture = Load_KTX("./resources/gllogodistsm.ktx");

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
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

		glUseProgram(m_program);
		glBindTextureUnit(0, m_logo_texture);
		glUniformMatrix3fv(0, 1, GL_FALSE, glm::value_ptr(glm::mat3(1.0f)));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
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
#endif //DISTANCE_FIELDS