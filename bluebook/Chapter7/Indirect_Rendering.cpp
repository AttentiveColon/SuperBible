#include "Defines.h"
#ifdef INDIRECT_RENDERING
#include "System.h"
#include "Model.h"
#include "Texture.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 3)
uniform mat4 mvp;

layout (location = 10)
in uint draw_id;

out vec2 vs_uv;

layout (location = 11)
uniform float time;

void main() 
{
	mat4 m1;
	mat4 m2;
	mat4 m;
	float t = time;
	float f = float(draw_id) / 30.0;

	float st = sin(t * 0.5 + f * 5.0);
	float ct = cos(t * 0.5 + f * 5.0);

	float j = fract(f);
	float d = cos(j * 3.14159);

	//rotate around y
	m[0] = vec4(ct, 0.0, st, 0.0);
	m[1] = vec4(0.0, 1.0, 0.0, 0.0);
	m[2] = vec4(-st, 0.0, ct, 0.0);
	m[3] = vec4(0.0, 0.0, 0.0, 1.0);

	//translate in the xy plane
	m1[0] = vec4(1.0, 0.0, 0.0, 0.0);
	m1[1] = vec4(0.0, 1.0, 0.0, 0.0);
	m1[2] = vec4(0.0, 0.0, 1.0, 0.0);
	m1[3] = vec4(260.0 + 30.0 * d, 5.0 * sin(f * 123.123), 0.0, 1.0);

	m = m * m1;

	//rotate around x
	st = sin(t * 2.1 * (600.0 + f) * 0.01);
	ct = cos(t * 2.1 * (600.0 + f) * 0.01);

	m1[0] = vec4(ct, st, 0.0, 0.0);
	m1[1] = vec4(-st, ct, 0.0, 0.0);
	m1[2] = vec4(0.0, 0.0, 1.0, 0.0);
	m1[3] = vec4(260.0 + 30.0 * d, 5.0 * sin(f + 123.123), 0.0, 1.0);

	m = m * m1;

	//rotate around z
	st = sin(t * 1.7 * (700.0 + f) * 0.01);
	ct = cos(t * 1.7 * (700.0 + f) * 0.01);

	m1[0] = vec4(1.0, 0.0, 0.0, 0.0);
	m1[1] = vec4(0.0, ct, st, 0.0);
	m1[2] = vec4(0.0, -st, ct, 0.0);
	m1[3] = vec4(0.0, 0.0, 0.0, 1.0);

	//m = m * m1;

	//non uniform scale
	float f1 = 0.65 + cos(f * 1.1) * 0.2;
	float f2 = 0.65 + cos(f * 1.1) * 0.2;
	float f3 = 0.65 + cos(f * 1.3) * 0.2;

	m1[0] = vec4(f1, 0.0, 0.0, 0.0);
	m1[1] = vec4(0.0, f2, 0.0, 0.0);
	m1[2] = vec4(0.0, 0.0, f3, 0.0);
	m1[3] = vec4(0.0, 0.0, 0.0, 1.0);

	//m = m * m1;
	
	gl_Position = mvp * m * vec4(position, 1.0);
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

in vec2 vs_uv;

out vec4 color;

void main() 
{
	color = vec4(vs_uv.x, vs_uv.y, 0.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLfloat square_vertices[] = {
		-0.500000, 0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 1.000000,
		0.500000, 0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 1.000000,
		-0.500000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 0.000000,
		0.500000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 0.000000
};

static const GLuint square_indices[] = {
		0, 1, 3,
		0, 3, 2
};

static const GLuint square_count = 6;
static const GLuint square_first_index = 0;
static const GLuint square_base_vertex = 0;

static const GLfloat triangle_vertices[] = {
		-0.500000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 1.000000,
		0.500000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 0.000000,
		0.000000, 0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 1.000000
};

static const GLuint triangle_indices[] = {
		1, 2, 0
};

static const GLuint triangle_count = 3;
static const GLuint triangle_first_index = 6;
static const GLuint triangle_base_vertex = 4;

static const GLfloat diamond_vertices[] = {
		-0.250000, 0.000000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 1.000000,
		0.000000, 0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 1.000000,
		0.000000, -0.500000, -0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 0.000000,
		0.250000, 0.000000, -0.000000, 0.000000, 0.000000, -1.000000, 1.000000, 0.000000
};

static const GLuint diamond_indices[] = {
		0, 1, 3,
		0, 3, 2
};

static const GLuint diamond_count = 6;
static const GLuint diamond_first_index = 9;
static const GLuint diamond_base_vertex = 7;

static const GLfloat trapezoid_vertices[] = {
		-0.500000, -0.500000, -0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 1.000000,
		0.000000, -0.500000, -0.000000, 0.000000, 0.000000, 1.000000, 1.000000, 1.000000,
		0.000000, 0.500000, -0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000,
		0.500000, 0.500000, -0.000000, 0.000000, 0.000000, 1.000000, 1.000000, 0.000000
};

static const GLuint trapezoid_indices[] = {
		0, 1, 3,
		0, 3, 2
};

static const GLuint trapezoid_count = 6;
static const GLuint trapezoid_first_index = 15;
static const GLuint trapezoid_base_vertex = 11;

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct DrawElementsIndirectCommand {
	GLuint count;
	GLuint instanceCount;
	GLuint firstIndex;
	GLuint baseVertex;
	GLuint baseInstance;
};



#define NUM_DRAWS 3000

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ibo;
	GLuint m_indirect_buffer;
	GLuint m_draw_index_buffer;

	SB::Camera m_camera;

	bool m_input_mode_active = true;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		input.SetRawMouseMode(window.GetHandle(), true);


		m_program = LoadShaders(shader_text);
		//SB::ModelDump model = SB::ModelDump("./resources/indirect_render_model.glb");
		m_camera = SB::Camera(std::string("Camera"), glm::vec3(0.0f, 2.0f, 5.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);


		//Create buffers and bind VAO
		glCreateVertexArrays(1, &m_vao);
		glCreateBuffers(1, &m_vbo);


		//Create buffer large enough to store all vertex data
		size_t buffer_size = sizeof(square_vertices) + sizeof(triangle_vertices) + sizeof(diamond_vertices) + sizeof(trapezoid_vertices);
		glNamedBufferData(m_vbo, buffer_size, NULL, GL_STATIC_DRAW);

		//Place data into buffers at appropriate offsets
		GLuint offset = 0;
		glNamedBufferSubData(m_vbo, offset, sizeof(square_vertices), square_vertices);
		offset += sizeof(square_vertices);
		glNamedBufferSubData(m_vbo, offset, sizeof(triangle_vertices), triangle_vertices);
		offset += sizeof(triangle_vertices);
		glNamedBufferSubData(m_vbo, offset, sizeof(diamond_vertices), diamond_vertices);
		offset += sizeof(diamond_vertices);
		glNamedBufferSubData(m_vbo, offset, sizeof(trapezoid_vertices), trapezoid_vertices);

		//Describe the locations of each array in the buffer
		glVertexArrayAttribBinding(m_vao, 0, 0);
		glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(m_vao, 1, 0);
		glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
		glVertexArrayAttribBinding(m_vao, 2, 0);
		glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
		
		//Enable those attributes
		glEnableVertexArrayAttrib(m_vao, 0);
		glEnableVertexArrayAttrib(m_vao, 1);
		glEnableVertexArrayAttrib(m_vao, 2);

		//Bind buffer to VAO
		glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(float) * 8);
		
		//Create buffer to store all indices data
		glCreateBuffers(1, &m_ibo);
		buffer_size = sizeof(square_indices) + sizeof(triangle_indices) + sizeof(diamond_indices) + sizeof(trapezoid_indices);
		glNamedBufferData(m_ibo, buffer_size, NULL, GL_STATIC_DRAW);

		//Place indice data
		offset = 0;
		glNamedBufferSubData(m_ibo, offset, sizeof(square_indices), square_indices);
		offset += sizeof(square_indices);
		glNamedBufferSubData(m_ibo, offset, sizeof(triangle_indices), triangle_indices);
		offset += sizeof(triangle_indices);
		glNamedBufferSubData(m_ibo, offset, sizeof(diamond_indices), diamond_indices);
		offset += sizeof(diamond_indices);
		glNamedBufferSubData(m_ibo, offset, sizeof(trapezoid_indices), trapezoid_indices);

		//Bind index buffer to VAO
		glVertexArrayElementBuffer(m_vao, m_ibo);

		//Create draw_id buffer
		glCreateBuffers(1, &m_draw_index_buffer);
		glNamedBufferData(m_draw_index_buffer, NUM_DRAWS * sizeof(GLuint), NULL, GL_STATIC_DRAW);

		GLuint* draw_index = (GLuint*)glMapNamedBufferRange(m_draw_index_buffer, 0, NUM_DRAWS * sizeof(GLuint), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		for (int i = 0; i < NUM_DRAWS; ++i) {
			draw_index[i] = i;
		}
		glUnmapNamedBuffer(m_draw_index_buffer);

		glVertexArrayAttribBinding(m_vao, 10, 0);
		glVertexArrayBindingDivisor(m_vao, 10, 1);
		glEnableVertexArrayAttrib(m_vao, 10);


		//Unbind buffers
		glBindVertexArray(0);

		//Create Indirect Buffer Data
		glCreateBuffers(1, &m_indirect_buffer);
		glNamedBufferData(m_indirect_buffer, NUM_DRAWS * sizeof(DrawElementsIndirectCommand), NULL, GL_STATIC_DRAW);

		DrawElementsIndirectCommand* cmd = 
			(DrawElementsIndirectCommand*)glMapNamedBufferRange(
				m_indirect_buffer, 
				0, 
				NUM_DRAWS * sizeof(DrawElementsIndirectCommand), 
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
			);

		for (int i = 0; i < NUM_DRAWS; ++i) {
			if (i % 4 == 0) {
				cmd[i].count = square_count;
				cmd[i].firstIndex = square_first_index;
				cmd[i].baseVertex = square_base_vertex;
			}
			else if (i % 4 == 1) {
				cmd[i].count = triangle_count;
				cmd[i].firstIndex = triangle_first_index;
				cmd[i].baseVertex = triangle_base_vertex;
			}
			else if (i % 4 == 2) {
				cmd[i].count = diamond_count;
				cmd[i].firstIndex = diamond_first_index;
				cmd[i].baseVertex = diamond_base_vertex;
			}
			else {
				cmd[i].count = trapezoid_count;
				cmd[i].firstIndex = trapezoid_first_index;
				cmd[i].baseVertex = trapezoid_base_vertex;
			}
			cmd[i].instanceCount = 1;
			cmd[i].baseInstance = i;
		}
		glUnmapNamedBuffer(m_indirect_buffer);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (m_input_mode_active) {
			m_camera.OnUpdate(input, 3.0f, 0.2f, dt);
		}

		//Implement Camera Movement Functions
		if (input.Pressed(GLFW_KEY_LEFT_CONTROL)) {
			m_input_mode_active = !m_input_mode_active;
			input.SetRawMouseMode(window.GetHandle(), m_input_mode_active);
		}


	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(m_camera.m_viewproj));
		glUniform1f(11, m_time);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);

		int time = (int)m_time;

		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, NUM_DRAWS, 0);

		////draw square
		//if (time % 4 == 0) glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * 0), 0);
		////draw triangle
		//else if (time % 4 == 1) glDrawElementsBaseVertex(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * 6), 4);
		////draw diamond
		//else if (time % 4 == 2) glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * 9), 7);
		////draw trapezoid
		////else glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * 15), 11);

		////glMultiDrawElementsIndirect()
		//else glDrawElementsInstancedBaseVertexBaseInstance(
		//	GL_TRIANGLES, 
		//	6,									//count
		//	GL_UNSIGNED_INT, 
		//	(void*)(sizeof(GLuint) * 15),		//firstIndex * sizeoftype
		//	1,									//instanceCount
		//	11,									//baseVertex
		//	0									//baseInstance
		//);
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
#endif //INDIRECT_RENDERING