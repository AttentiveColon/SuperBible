#include "Defines.h"
#ifdef CUBIC_BEZIER_PATCHES
#include "System.h"
#include "Texture.h"
#include "Model.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

layout (location = 10)
uniform mat4 mv_matrix;

layout (location = 12)
uniform mat4 mvp;

void main()
{
	gl_Position = mv_matrix * vec4(position, 1.0);
}
)";

static const GLchar* tess_control_shader_source = R"(
#version 450 core

layout (vertices = 16) out;

void main(void)
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = 16.0;
		gl_TessLevelInner[1] = 16.0;
		gl_TessLevelOuter[0] = 16.0;
		gl_TessLevelOuter[1] = 16.0;
		gl_TessLevelOuter[2] = 16.0;
		gl_TessLevelOuter[3] = 16.0;
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

static const GLchar* tess_evaluation_shader_source = R"(
#version 450 core

layout (quads, equal_spacing, cw) in;

layout (location = 11)
uniform mat4 proj_matrix;

out TES_OUT
{
	vec3 N;
} tes_out;

vec4 quadratic_bezier(vec4 A, vec4 B, vec4 C, float t)
{
	vec4 D = mix(A, B, t);
	vec4 E = mix(B, C, t);
	return mix(D, E , t);
}

vec4 cubic_bezier(vec4 A, vec4 B, vec4 C, vec4 D, float t)
{
	vec4 E = mix(A, B, t);
	vec4 F = mix(B, C, t);
	vec4 G = mix(C, D, t);
	return quadratic_bezier(E, F, G, t);
}

vec4 evaluate_patch(vec2 at)
{
	vec4 P[4];
	int i;

	for (i = 0; i < 4; ++i)
	{
		P[i] = cubic_bezier(gl_in[i+0].gl_Position,
							gl_in[i+4].gl_Position,
							gl_in[i+8].gl_Position,
							gl_in[i+12].gl_Position,
							at.y);
	}

	return cubic_bezier(P[0], P[1], P[2], P[3], at.x);
}

const float epsilon = 0.001;

void main(void)
{
	vec4 p1 = evaluate_patch(gl_TessCoord.xy);
	vec4 p2 = evaluate_patch(gl_TessCoord.xy + vec2(0.0, epsilon));
	vec4 p3 = evaluate_patch(gl_TessCoord.xy + vec2(epsilon, 0.0));

	vec3 v1 = normalize(p2.xyz - p1.xyz);
	vec3 v2 = normalize(p3.xyz - p1.xyz);

	tes_out.N = cross(v1, v2);

	gl_Position = proj_matrix * p1;
}
)";


static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

in TES_OUT
{
	vec3 N;
} fs_in;

void main() 
{
	vec3 N = normalize(fs_in.N);

	vec4 c = vec4(1.0, -1.0, 0.0, 0.0) * N.z + vec4(0.0, 0.0, 0.0, 1.0);
	color = clamp(c, vec4(0.0), vec4(1.0));
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_TESS_CONTROL_SHADER, tess_control_shader_source, NULL},
	{GL_TESS_EVALUATION_SHADER, tess_evaluation_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* cp_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

layout (location = 12)
uniform mat4 mvp;

void main()
{
	gl_Position = mvp * vec4(position, 1.0);
}
)";

static const GLchar* cp_fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main()
{
	color = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static ShaderText cp_shader_text[] = {
	{GL_VERTEX_SHADER, cp_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, cp_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};


struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_cp_program;

	GLuint m_vao;
	GLuint m_patch_buffer;
	GLuint m_cage_indices;

	glm::vec3 m_patch_data[16];

	bool m_wireframe = false;



	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);

		m_program = LoadShaders(shader_text);
		m_cp_program = LoadShaders(cp_shader_text);

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		glGenBuffers(1, &m_patch_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_patch_buffer);

		glBufferData(GL_ARRAY_BUFFER, sizeof(m_patch_data), NULL, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);

		static const GLushort indices[] =
		{
			0, 1, 1, 2, 2, 3,
			4, 5, 5, 6, 6, 7,
			8, 9, 9, 10, 10, 11,
			12, 13, 13, 14, 14, 15,

			0, 4, 4, 8, 8, 12,
			1, 5, 5, 9, 9, 13,
			2, 6, 6, 10, 10, 14,
			3, 7, 7, 11, 11, 15
		};

		glGenBuffers(1, &m_cage_indices);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cage_indices);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const float patch_initializer[] =
		{
			-1.0f,  -1.0f,  0.0f,
			-0.33f, -1.0f,  0.0f,
			 0.33f, -1.0f,  0.0f,
			 1.0f,  -1.0f,  0.0f,

			-1.0f,  -0.33f, 0.0f,
			-0.33f, -0.33f, 0.0f,
			 0.33f, -0.33f, 0.0f,
			 1.0f,  -0.33f, 0.0f,

			-1.0f,   0.33f, 0.0f,
			-0.33f,  0.33f, 0.0f,
			 0.33f,  0.33f, 0.0f,
			 1.0f,   0.33f, 0.0f,

			-1.0f,   1.0f,  0.0f,
			-0.33f,  1.0f,  0.0f,
			 0.33f,  1.0f,  0.0f,
			 1.0f,   1.0f,  0.0f,
		};

		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glm::vec3* p = (glm::vec3*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(m_patch_data), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		memcpy(p, patch_initializer, sizeof(patch_initializer));

		for (int i = 0; i < 16; i++) {
			float fi = (float)i / 16.0f;
			p[i][2] = glm::sin(m_time * (0.2f + fi * 0.3f));
		}
		
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glBindBuffer(GL_ARRAY_BUFFER, m_patch_buffer);
		glBindVertexArray(m_vao);
		glUseProgram(m_program);

		glm::mat4 proj_matrix = glm::perspective(0.90f, 16.0f / 9.0f, 0.001f, 1000.0f);
		glm::mat4 mv_matrix = glm::translate(glm::vec3(0.0f, 0.0f, -4.0f)) * glm::rotate((float)m_time * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate((float)m_time * glm::radians(17.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		glUniformMatrix4fv(10, 1, GL_FALSE, glm::value_ptr(mv_matrix));
		glUniformMatrix4fv(11, 1, GL_FALSE, glm::value_ptr(proj_matrix));

		if (m_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		
		glPatchParameteri(GL_PATCH_VERTICES, 16);
		glDrawArrays(GL_PATCHES, 0, 16);


		glUseProgram(m_cp_program);
		glUniformMatrix4fv(12, 1, GL_FALSE, glm::value_ptr(proj_matrix * mv_matrix));
		glPointSize(9.0f);
		//Render control points
		glDrawArrays(GL_POINTS, 0, 16);
		//Render lines
		glDrawElements(GL_LINES, 48, GL_UNSIGNED_SHORT, NULL);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
#endif //CUBIC_BEZIER_PATCHES
