#include "Defines.h"
#ifdef TRANSFORM_FEEDBACK_SPRINGMASS
#include "System.h"
#include "Model.h"
#include "Texture.h"

static const GLchar* vertex_update_program_source = R"(
#version 450 core

layout (location = 0)
in vec4 position_mass;

layout (location = 1)
in vec3 velocity;

layout (location = 2)
in ivec4 connection;

layout (binding = 0)
uniform samplerBuffer tex_position;

out vec4 tf_position_mass;
out vec3 tf_velocity;

//layout (location = 3)
uniform float t = 0.07;

//Spring constant
uniform float k = 7.1;

//Gravity
const vec3 gravity = vec3(0.0, -0.005, 0.0);

//Damping constant
uniform float c = 2.8;

//Spring resting length
uniform float rest_length = 0.018;

void main()
{
	vec3 p = position_mass.xyz;
	float m = position_mass.w;
	vec3 u = velocity;
	vec3 F = gravity * m - c * u;
	bool fixed_node = true;

	for (int i = 0; i < 4; ++i) 
	{
		if (connection[i] != -1)
		{
			vec3 q = texelFetch(tex_position, connection[i]).xyz;
			vec3 d = q - p;
			float x = length(d);
			F += -k * (rest_length - x) * normalize(d);
			fixed_node = false;
		}
	}

	if (fixed_node)
	{
		F = vec3(0.0);
	}

	vec3 a = F / m;

	vec3 s = u * t + 0.5 * a * t * t;
	
	vec3 v = u + a * t;

	s = clamp(s, vec3(-25.0), vec3(25.0));

	tf_position_mass = vec4(p + s, m);
	tf_velocity = v;
}
)";

static ShaderText update_shader_text[] = {
	{GL_VERTEX_SHADER, vertex_update_program_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* vertex_render_shader_source = R"(
#version 450 core

layout (location = 0)
in vec4 position_mass;

layout (location = 1)
in vec3 velocity;

layout (location = 2)
in ivec4 connection;

layout (binding = 0)
uniform samplerBuffer tex_position;

layout (location = 3)
uniform float t;

void main() 
{
	gl_Position = vec4(position_mass.xyz, 1.0);
}
)";

static const GLchar* fragment_render_shader_source = R"(
#version 450 core

out vec4 color;

void main() 
{
	color = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static ShaderText render_shader_text[] = {
	{GL_VERTEX_SHADER, vertex_render_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_render_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

enum BUFFER_TYPE_t {
	POSITION_A,
	POSITION_B,
	VELOCITY_A,
	VELOCITY_B,
	CONNECTION
};

enum
{
	POINTS_X = 50,
	POINTS_Y = 50,
	POINTS_TOTAL = (POINTS_X * POINTS_Y),
	CONNECTIONS_TOTAL = (POINTS_X - 1) * POINTS_Y * (POINTS_Y - 1) * POINTS_X
};

void SetFeedbackVaryings(GLuint program, int count, const char* const* varyings, GLenum mode) {
	glTransformFeedbackVaryings(program, count, varyings, mode);
	glLinkProgram(program);
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_vao[2];
	GLuint m_vbo[5];
	GLuint m_index_buffer;
	GLuint m_pos_tbo[2];
	GLuint m_update_program;
	GLuint m_render_program;
	GLuint m_C_loc;
	GLuint m_iteration_index;

	bool draw_points;
	bool draw_lines;
	int iterations_per_frame;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0),
		m_iteration_index(0),
		m_update_program(0),
		m_render_program(0),
		draw_points(true),
		draw_lines(true),
		iterations_per_frame(16)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);

		m_update_program = LoadShaders(update_shader_text);
		static const char* tf_varyings[] = {
			"tf_position_mass",
			"tf_velocity"
		};
		SetFeedbackVaryings(m_update_program, 2, tf_varyings, GL_SEPARATE_ATTRIBS);
		m_render_program = LoadShaders(render_shader_text);

		glm::vec4* initial_positions = new glm::vec4[POINTS_TOTAL];
		glm::vec3* initial_velocities = new glm::vec3[POINTS_TOTAL];
		glm::ivec4* connection_vectors = new glm::ivec4[POINTS_TOTAL];

		int n = 0;

		for (int j = 0; j < POINTS_Y; ++j) {
			float fj = (float)j / (float)POINTS_Y;
			for (int i = 0; i < POINTS_X; ++i) {
				float fi = (float)i / (float)POINTS_X;

				
				initial_positions[n] = glm::vec4(
					fi - 0.5, fj - 0.1, 0.6f * glm::sin(fi) * glm::cos(fj), 1.0f
				);
				/*initial_positions[n] = glm::vec4(
					(fi - 0.5f) * (float)POINTS_X,
					(fj - 0.5f) * (float)POINTS_Y,
					0.6f * glm::sin(fi) * glm::cos(fj),
					1.0f
				);*/

				//std::cout << initial_positions[n].x << " " << initial_positions[n].y << " " << initial_positions[n].z << std::endl;


				if (j < POINTS_Y / 2) {
					initial_velocities[n] = glm::vec3(1.0f, 1.0, 0.0);
				}
				else {
					initial_velocities[n] = glm::vec3(0.0f);
				}

				connection_vectors[n] = glm::ivec4(-1);

				if (j != (POINTS_Y - 1)) {
					if (i != 0) connection_vectors[n][0] = n - 1;
					if (j != 0) connection_vectors[n][1] = n - POINTS_X;
					if (i != (POINTS_X - 1)) connection_vectors[n][2] = n + 1;
					if (j != (POINTS_Y - 1)) connection_vectors[n][3] = n + POINTS_X;
				}
				n++;
			}
		}

		glGenVertexArrays(2, m_vao);
		glGenBuffers(5, m_vbo);

		for (int i = 0; i < 2; i++) {
			glBindVertexArray(m_vao[i]);

			glBindBuffer(GL_ARRAY_BUFFER, m_vbo[POSITION_A + i]);
			glBufferData(GL_ARRAY_BUFFER, POINTS_TOTAL * sizeof(glm::vec4), initial_positions, GL_DYNAMIC_COPY);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(0);

			glBindBuffer(GL_ARRAY_BUFFER, m_vbo[VELOCITY_A + i]);
			glBufferData(GL_ARRAY_BUFFER, POINTS_TOTAL * sizeof(glm::vec3), initial_velocities, GL_DYNAMIC_COPY);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(1);

			glBindBuffer(GL_ARRAY_BUFFER, m_vbo[CONNECTION]);
			glBufferData(GL_ARRAY_BUFFER, POINTS_TOTAL * sizeof(glm::ivec4), connection_vectors, GL_STATIC_DRAW);
			glVertexAttribIPointer(2, 4, GL_INT, 0, NULL);
			glEnableVertexAttribArray(2);
		}

		delete[] connection_vectors;
		delete[] initial_velocities;
		delete[] initial_positions;

		glGenTextures(2, m_pos_tbo);
		glBindTexture(GL_TEXTURE_BUFFER, m_pos_tbo[0]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_vbo[POSITION_A]);
		glBindTexture(GL_TEXTURE_BUFFER, m_pos_tbo[1]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_vbo[POSITION_B]);

		int lines = (POINTS_X - 1) * POINTS_Y + (POINTS_Y - 1) * POINTS_X;

		glGenBuffers(1, &m_index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, lines * 2 * sizeof(int), NULL, GL_STATIC_DRAW);

		int* e = (int*)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, lines * 2 * sizeof(int), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

		for (int j = 0; j < POINTS_Y; ++j) {
			for (int i = 0; i < POINTS_X - 1; ++i) {
				*e++ = i + j * POINTS_X;
				*e++ = 1 + i + j * POINTS_X;
			}
		}

		for (int i = 0; i < POINTS_X; ++i) {
			for (int j = 0; j < POINTS_Y - 1; ++j) {
				*e++ = i + j * POINTS_X;
				*e++ = POINTS_X + i + j * POINTS_X;
			}
		}

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();


	}
	void OnDraw() {
		

		//glUniform1f(3, (float)m_time);

		glUseProgram(m_update_program);
		glEnable(GL_RASTERIZER_DISCARD);

		for (int i = iterations_per_frame; i != 0; --i) {
			glBindVertexArray(m_vao[m_iteration_index & 1]);
			glBindTexture(GL_TEXTURE_BUFFER, m_pos_tbo[m_iteration_index & 1]);
			m_iteration_index++;
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_vbo[POSITION_A + (m_iteration_index & 1)]);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_vbo[VELOCITY_A + (m_iteration_index & 1)]);
			glBeginTransformFeedback(GL_POINTS);
			glDrawArrays(GL_POINTS, 0, POINTS_TOTAL);
			glEndTransformFeedback();
		}

		glDisable(GL_RASTERIZER_DISCARD);

		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_render_program);

		if (draw_points) {
			glPointSize(4.0f);
			glDrawArrays(GL_POINTS, 0, POINTS_TOTAL);
		}

		if (draw_lines) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
			glDrawElements(GL_LINES, CONNECTIONS_TOTAL * 2, GL_UNSIGNED_INT, NULL);
		}
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
#endif //TRANSFORM_FEEDBACK_SPRINGMASS