#include "Defines.h"
#ifdef TESSELATION_MODES
#include "System.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 positions;

void main() 
{
	gl_Position = vec4(positions, 1.0);
}
)";

static const GLchar* tess_control_shader_source = R"(
#version 450 core

layout (vertices = 4) out;

void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelInner[0] = 9.0;
		gl_TessLevelInner[1] = 7.0;
		gl_TessLevelOuter[0] = 3.0;
		gl_TessLevelOuter[1] = 5.0;
		gl_TessLevelOuter[2] = 3.0;
		gl_TessLevelOuter[3] = 5.0;
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

static const GLchar* tess_evaluation_shader_source = R"(
#version 450 core

layout (quads) in;

void main()
{
	vec4 p1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 p2 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
	gl_Position = mix(p1, p2, gl_TessCoord.y);
}
)";

static const GLchar* tess_tri_control_shader_source = R"(
#version 450 core

layout (vertices = 3) out;

void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelInner[0] = 5.0;
		gl_TessLevelOuter[0] = 8.0;
		gl_TessLevelOuter[1] = 8.0;
		gl_TessLevelOuter[2] = 8.0;
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

static const GLchar* tess_tri_evaluation_shader_source = R"(
#version 450 core

layout (triangles) in;

void main()
{
	gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) + (gl_TessCoord.y * gl_in[1].gl_Position) + (gl_TessCoord.z * gl_in[2].gl_Position);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main() 
{
	color = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_TESS_CONTROL_SHADER, tess_control_shader_source, NULL},
	{GL_TESS_EVALUATION_SHADER, tess_evaluation_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static ShaderText shader_tri_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_TESS_CONTROL_SHADER, tess_tri_control_shader_source, NULL},
	{GL_TESS_EVALUATION_SHADER, tess_tri_evaluation_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static ShaderText shader_text_no_tess[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};


static GLuint GetQuad() {

	GLuint vao, vbo, ebo;

	static const GLfloat quad[] = {
		-0.5, 0.5, 0.5,		//Top Left
		0.5, 0.5, 0.5,		//Top Right
		-0.5, -0.5, 0.5,	//Bottom Left
		0.5, -0.5, 0.5,		//Bottom Right
	};
	
	static const GLint quad_elements[] = {
		0, 1, 2,
		2, 1, 3,
	};

	static const GLint quad_line_elements[] = {
		0, 1,
		0, 2,
		1, 3,
		3, 2,
		2, 1
	};

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vbo);
	glCreateBuffers(1, &ebo);

	glNamedBufferStorage(vbo, sizeof(quad), quad, 0);

	glNamedBufferData(ebo, sizeof(quad_elements) + sizeof(quad_line_elements), NULL, GL_STATIC_DRAW);
	glNamedBufferSubData(ebo, 0, sizeof(quad_elements), quad_elements);
	glNamedBufferSubData(ebo, sizeof(quad_elements), sizeof(quad_line_elements), quad_line_elements);

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(vao, 0);

	glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 3);
	glVertexArrayElementBuffer(vao, ebo);

	return vao;
}

enum TessMode {
	Quad,
	Tri,
	None,
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_tess_quad_program;
	GLuint m_tess_tri_program;

	TessMode m_mode;

	GLuint m_vao;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0),
		m_mode(TessMode::Quad)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		glPointSize(15.0f);
		m_program = LoadShaders(shader_text_no_tess);
		m_tess_quad_program = LoadShaders(shader_text);
		m_tess_tri_program = LoadShaders(shader_tri_text);
		m_vao = GetQuad();
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(m_vao);

		if (m_mode == TessMode::Quad) {
			glUseProgram(m_tess_quad_program);
			glPatchParameteri(GL_PATCH_VERTICES, 4);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawArrays(GL_PATCHES, 0, 4);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		if (m_mode == TessMode::Tri) {
			glUseProgram(m_tess_tri_program);
			glPatchParameteri(GL_PATCH_VERTICES, 3);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawElements(GL_PATCHES, 6, GL_UNSIGNED_INT, 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glUseProgram(m_program);
		glDrawElements(GL_POINTS, 6, GL_UNSIGNED_INT, NULL);
		if (m_mode == TessMode::None) {
			glDrawElements(GL_LINES, 10, GL_UNSIGNED_INT, (GLvoid*)(sizeof(GLint) * 6));
		}
		

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		if (m_mode == 0) {
			ImGui::Text("Quad");
		}
		else if (m_mode == 1) {
			ImGui::Text("Tri");
		}
		else {
			ImGui::Text("None");
		}
		ImGui::SliderInt("Mode", (int*)&m_mode, 0, 2);
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
#endif //TESSELATION_MODES
