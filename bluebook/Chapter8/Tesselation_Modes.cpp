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

layout (binding = 0, std140)
uniform TessUniform
{
	float u_quad_inner_0;
	float u_quad_inner_1;
	float u_quad_outer_0;
	float u_quad_outer_1;
	float u_quad_outer_2;
	float u_quad_outer_3;

	float u_tri_inner_0;
	float u_tri_outer_0;
	float u_tri_outer_1;
	float u_tri_outer_2;

	float u_iso_outer_0;
	float u_iso_outer_1;
};

void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelInner[0] = u_quad_inner_0;
		gl_TessLevelInner[1] = u_quad_inner_1;
		gl_TessLevelOuter[0] = u_quad_outer_0;
		gl_TessLevelOuter[1] = u_quad_outer_1;
		gl_TessLevelOuter[2] = u_quad_outer_2;
		gl_TessLevelOuter[3] = u_quad_outer_3;
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

layout (binding = 0, std140)
uniform TessUniform
{
	float u_quad_inner_0;
	float u_quad_inner_1;
	float u_quad_outer_0;
	float u_quad_outer_1;
	float u_quad_outer_2;
	float u_quad_outer_3;

	float u_tri_inner_0;
	float u_tri_outer_0;
	float u_tri_outer_1;
	float u_tri_outer_2;

	float u_iso_outer_0;
	float u_iso_outer_1;
};

out float tc_offset[];

out patch float tc_offset_patch;

void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelInner[0] = u_tri_inner_0;
		gl_TessLevelOuter[0] = u_tri_outer_0;
		gl_TessLevelOuter[1] = u_tri_outer_1;
		gl_TessLevelOuter[2] = u_tri_outer_2;
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	tc_offset[gl_InvocationID] = 0.1;
	tc_offset_patch = 0.2;
}
)";

static const GLchar* tess_tri_evaluation_shader_source = R"(
#version 450 core

layout (triangles) in;

in float tc_offset[];
in patch float tc_offset_patch;

void main()
{
	gl_Position = ((tc_offset[0] + gl_TessCoord.x) * gl_in[0].gl_Position) + ((tc_offset_patch + gl_TessCoord.y) * gl_in[1].gl_Position) + (gl_TessCoord.z * gl_in[2].gl_Position);
}
)";

static const GLchar* tess_iso_control_shader_source = R"(
#version 450 core

layout (vertices = 4) out;

layout (binding = 0, std140)
uniform TessUniform
{
	float u_quad_inner_0;
	float u_quad_inner_1;
	float u_quad_outer_0;
	float u_quad_outer_1;
	float u_quad_outer_2;
	float u_quad_outer_3;

	float u_tri_inner_0;
	float u_tri_outer_0;
	float u_tri_outer_1;
	float u_tri_outer_2;

	float u_iso_outer_0;
	float u_iso_outer_1;
};

void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelOuter[0] = u_iso_outer_0;
		gl_TessLevelOuter[1] = u_iso_outer_1;
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

static const GLchar* tess_iso_evaluation_shader_source = R"(
#version 450 core

layout (isolines) in;

void main()
{
	float r = (gl_TessCoord.y + gl_TessCoord.x / gl_TessLevelOuter[0]);
	float t = gl_TessCoord.x * 2.0 * 3.14159;
	gl_Position = vec4(sin(t) * r, cos(t) * r, 0.5, 1.0);
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

static ShaderText shader_iso_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_TESS_CONTROL_SHADER, tess_iso_control_shader_source, NULL},
	{GL_TESS_EVALUATION_SHADER, tess_iso_evaluation_shader_source, NULL},
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

struct TessUniformBlock {
	//Quads
	float quad_inner_0;
	float quad_inner_1;
	float quad_outer_0;
	float quad_outer_1;
	float quad_outer_2;
	float quad_outer_3;

	//Triangles
	float tri_inner_0;
	float tri_outer_0;
	float tri_outer_1;
	float tri_outer_2;

	//Isolines
	float iso_outer_0;
	float iso_outer_1;
};

enum TessMode {
	Quad,
	Tri,
	Iso,
	None,
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_tess_quad_program;
	GLuint m_tess_tri_program;
	GLuint m_tess_iso_program;

	TessMode m_mode;

	GLuint m_vao;

	GLuint m_ubo;
	GLbyte* m_ubo_data;

	TessUniformBlock m_tess_ubo;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0),
		m_mode(TessMode::Quad),
		m_tess_ubo{9.0, 7.0, 3.0, 5.0, 3.0, 5.0, 5.0, 8.0, 8.0, 8.0, 5.0, 5.0 }
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		glPointSize(15.0f);
		m_program = LoadShaders(shader_text_no_tess);
		m_tess_quad_program = LoadShaders(shader_text);
		m_tess_tri_program = LoadShaders(shader_tri_text);
		m_tess_iso_program = LoadShaders(shader_iso_text);
		m_vao = GetQuad();

		glCreateBuffers(1, &m_ubo);
		m_ubo_data = new GLbyte[sizeof(TessUniformBlock)];
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		memcpy(m_ubo_data, &m_tess_ubo, sizeof(TessUniformBlock));
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(TessUniformBlock), m_ubo_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);

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

		if (m_mode == TessMode::Iso) {
			glUseProgram(m_tess_iso_program);
			glPatchParameteri(GL_PATCH_VERTICES, 4);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawArrays(GL_PATCHES, 0, 4);
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
			ImGui::SliderInt("Mode", (int*)&m_mode, 0, 3);
			float* q_inners[] = { &m_tess_ubo.quad_inner_0, &m_tess_ubo.quad_inner_1 };
			ImGui::DragFloat2("Inners", *q_inners, 0.1f, 1.0, 10.0f);
			float* q_outers[] = { &m_tess_ubo.quad_outer_0, &m_tess_ubo.quad_outer_1, &m_tess_ubo.quad_outer_2, &m_tess_ubo.quad_outer_3 };
			ImGui::DragFloat4("Outers", *q_outers, 0.1f, 1.0f, 10.0f);
		}
		else if (m_mode == 1) {
			ImGui::Text("Tri");
			ImGui::SliderInt("Mode", (int*)&m_mode, 0, 3);
			float* t_inners[] = { &m_tess_ubo.tri_inner_0 };
			ImGui::DragFloat("Inners", *t_inners, 0.1f, 1.0f, 10.0f);
			float* t_outers[] = { &m_tess_ubo.tri_outer_0, &m_tess_ubo.tri_outer_1, &m_tess_ubo.tri_outer_2 };
			ImGui::DragFloat3("Outers", *t_outers, 0.1f, 1.0f, 10.0f);
		} 
		else if (m_mode == 2) {
			ImGui::Text("Iso");
			ImGui::SliderInt("Mode", (int*)&m_mode, 0, 3);
			float* i_outers[] = { &m_tess_ubo.iso_outer_0, &m_tess_ubo.iso_outer_1 };
			ImGui::DragFloat2("Outers", *i_outers, 0.1f, 1.0f, 10.0f);
		}
		else {
			ImGui::Text("None");
			ImGui::SliderInt("Mode", (int*)&m_mode, 0, 3);
		}
		
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
