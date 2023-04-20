#include "Defines.h"
#ifdef MULTI_VIEWPORT
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (std140, binding = 0)
uniform default_block
{
	mat4 mvp_matrix[4];
};

layout (location = 9)
uniform mat4 mvp;
layout (location = 13)
uniform int viewport_index;

uniform int vid_offset = 0;

out VS_OUT
{
    vec4 color;
} vs_out;

void main(void)
{
    const vec4 vertices[] = vec4[](vec4(-0.5, -0.5, 0.0, 1.0),
                                   vec4( 0.5, -0.5, 0.0, 1.0),
                                   vec4( 0.5,  0.5, 0.0, 1.0),
                                   vec4(-0.5,  0.5, 0.0, 1.0));

    const vec4 colors[] = vec4[](vec4(0.0, 0.0, 0.0, 1.0),
                                 vec4(0.0, 0.0, 0.0, 1.0),
                                 vec4(0.0, 0.0, 0.0, 1.0),
                                 vec4(1.0, 1.0, 1.0, 1.0));

    gl_Position = mvp_matrix[viewport_index] * vertices[(gl_VertexID + vid_offset) % 4];
    vs_out.color = colors[gl_VertexID];
}
)";

static const GLchar* geometry_shader_source = R"(
#version 450 core

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 6) out;

layout (location = 13)
uniform int viewport_index;

in VS_OUT
{
    vec4 color;
} gs_in[4];

out GS_OUT
{
    flat vec4 color[4];
    vec2 uv;
} gs_out;

void main(void)
{

	gl_ViewportIndex = viewport_index;

	gl_Position = gl_in[0].gl_Position;
	gs_out.uv = vec2(1.0, 0.0);
	EmitVertex();

	gl_Position = gl_in[1].gl_Position;
	gs_out.uv = vec2(0.0, 0.0);
	EmitVertex();

	gl_Position = gl_in[2].gl_Position;
	gs_out.uv = vec2(0.0, 1.0);

	const int idx0 = 0;
	const int idx1 = 1;
	const int idx2 = 2;
	const int idx3 = 3;

	// We're only writing the output color for the last
	// vertex here because they're flat attributes,
	// and the last vertex is the provoking vertex by default
	gs_out.color[0] = gs_in[idx0].color;
	gs_out.color[1] = gs_in[idx1].color;
	gs_out.color[2] = gs_in[idx2].color;
	gs_out.color[3] = gs_in[idx3].color;
	EmitVertex();

	gl_Position = gl_in[0].gl_Position;
	gs_out.uv = vec2(1.0, 0.0);
	gs_out.color[0] = gs_in[idx0].color;
	gs_out.color[1] = gs_in[idx1].color;
	gs_out.color[2] = gs_in[idx2].color;
	gs_out.color[3] = gs_in[idx3].color;
	EmitVertex();

	gl_Position = gl_in[2].gl_Position;
	gs_out.uv = vec2(0.0, 1.0);
	gs_out.color[0] = gs_in[idx0].color;
	gs_out.color[1] = gs_in[idx1].color;
	gs_out.color[2] = gs_in[idx2].color;
	gs_out.color[3] = gs_in[idx3].color;
	EmitVertex();

	gl_Position = gl_in[3].gl_Position;
	gs_out.uv = vec2(1.0, 1.0);
	gs_out.color[0] = gs_in[idx0].color;
	gs_out.color[1] = gs_in[idx1].color;
	gs_out.color[2] = gs_in[idx2].color;
	gs_out.color[3] = gs_in[idx3].color;
	EmitVertex();

	EndPrimitive();
	
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in GS_OUT
{
    flat vec4 color[4];
    vec2 uv;
} fs_in;

out vec4 color;

void main(void)
{
    vec4 c1 = mix(fs_in.color[0], fs_in.color[1], fs_in.uv.x);
    vec4 c2 = mix(fs_in.color[2], fs_in.color[3], fs_in.uv.x);

    color = mix(c1, c2, fs_in.uv.y);
}

)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_GEOMETRY_SHADER, geometry_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};




struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	WindowXY m_window_size;


	GLuint m_program;
	GLuint m_vao;
	GLuint m_uniform_buffer;

	SB::Camera m_camera[4];
	int m_active_window = 0;

	bool m_wireframe = false;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glFrontFace(GL_CCW);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);


		m_program = LoadShaders(shader_text);

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);


		m_camera[0] = SB::Camera("Camera", glm::vec3(1.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);
		m_camera[1] = SB::Camera("Camera", glm::vec3(1.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);
		m_camera[2] = SB::Camera("Camera", glm::vec3(1.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);
		m_camera[3] = SB::Camera("Camera", glm::vec3(1.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);

		glGenBuffers(1, &m_uniform_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, m_uniform_buffer);
		glBufferData(GL_UNIFORM_BUFFER, 4 * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
		m_window_size = window.GetWindowDimensions();

		float viewport_width = (float)(7 * m_window_size.width) / 16.0f;
		float viewport_height = (float)(7 * m_window_size.height) / 16.0f;

		if (input.MousePressed(GLFW_MOUSE_BUTTON_3)) {
			MousePos mp = input.GetMousePos();

			if ((0 < mp.x && mp.x < viewport_width) && (m_window_size.height - viewport_height < mp.y && mp.y < m_window_size.height)) m_active_window = 0;
			else if ((m_window_size.width - viewport_width < mp.x && mp.x < m_window_size.width) && (m_window_size.height - viewport_height < mp.y && mp.y < m_window_size.height)) m_active_window = 1;
			else if ((0 < mp.x && mp.x < viewport_width) && (0 < mp.y && mp.y < viewport_height)) m_active_window = 2;
			else if ((m_window_size.width - viewport_width < mp.x && mp.x < m_window_size.width) && (0 < mp.y && mp.y < viewport_height)) m_active_window = 3;
			else m_active_window = -1;
		}
		if (input.MouseHeld(GLFW_MOUSE_BUTTON_3)) {
			if (m_active_window != -1) {
				input.SetRawMouseMode(window.GetHandle(), true);
				m_camera[m_active_window].OnUpdate(input, 3.0f, 0.2f, dt);
			}
		}
		else {
			input.SetRawMouseMode(window.GetHandle(), false);
		}

		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_uniform_buffer);
		glm::mat4* mvp_array = (glm::mat4*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, 4 * sizeof(glm::mat4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		for (int i = 0; i < 4; ++i) {
			mvp_array[i] = m_camera[i].ViewProj();
		}
		glUnmapBuffer(GL_UNIFORM_BUFFER);
		
	}
	void OnDraw() {
		glViewport(0, 0, m_window_size.width, m_window_size.height);
		glClearBufferfv(GL_COLOR, 0, glm::value_ptr(glm::vec4(0.0)));
		glClear(GL_DEPTH_BUFFER_BIT);

		float viewport_width = (float)(7 * m_window_size.width) / 16.0f;
		float viewport_height = (float)(7 * m_window_size.height) / 16.0f;

		//lower left
		glViewportIndexedf(0, 0, 0, viewport_width, viewport_height);
		glScissor(0, 0, viewport_width, viewport_height);
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		//lower right
		glViewportIndexedf(1, m_window_size.width - viewport_width, 0, viewport_width, viewport_height);
		glScissor(m_window_size.width - viewport_width, 0, viewport_width, viewport_height);
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0.1f, 0.2f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		//upper left
		glViewportIndexedf(2, 0, m_window_size.height - viewport_height, viewport_width, viewport_height);
		glScissor(0, m_window_size.height - viewport_height, viewport_width, viewport_height);
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		//upper right
		glViewportIndexedf(3, m_window_size.width - viewport_width, m_window_size.height - viewport_height, viewport_width, viewport_height);
		glScissor(m_window_size.width - viewport_width, m_window_size.height - viewport_height, viewport_width, viewport_height);
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0.2f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);

		if (m_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glUseProgram(m_program);


		glUniformMatrix4fv(9, 1, GL_FALSE, glm::value_ptr(m_camera[0].ViewProj()));
		for (int i = 0; i < 4; ++i) {
			glUniform1i(13, i);
			glDrawArrays(GL_LINES_ADJACENCY, 0, 4);
		}

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
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
#endif //MULTI_VIEWPORT
