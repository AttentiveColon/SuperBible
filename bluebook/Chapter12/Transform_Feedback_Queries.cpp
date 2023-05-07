#include "Defines.h"
#ifdef TRANSFORM_FEEDBACK_QUERIES
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* transform_feedback_vertex_shader_source = R"(
#version 450 core

out vec3 vPosition;

void main(void)
{
	const vec3 offsets[] = vec3[] (vec3(-5.0, 0.0, 0.0), vec3(5.0, 0.0, 0.0), vec3(0.0, 5.0, 0.0));
    const vec3 positions[] = vec3[] (vec3(-0.5, -0.5, 0.0), vec3(0.0, 0.5, 0.0), vec3(0.5, -0.5, 0.0));

	int curr_triangle = gl_VertexID / 3;
	vPosition = positions[gl_VertexID % 3] + offsets[curr_triangle];
}
)";

static ShaderText transform_feedback_shader_text[] = {
	{GL_VERTEX_SHADER, transform_feedback_vertex_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

layout (location = 1)
uniform mat4 mvp;

void main(void)
{
	gl_Position = mvp * vec4(position, 1.0);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main(void)
{
	color = vec4(1.0);
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

	GLuint m_transform_vao;
	GLuint m_transform_feedback_program;
	GLuint m_queries;
	GLuint m_feedback_buffer;
	GLuint m_result;

	GLuint m_program;
	GLuint m_vao;
	SB::Camera m_camera;
	bool m_input_mode = false;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glGenQueries(1, &m_queries);

		glGenVertexArrays(1, &m_transform_vao);
		glBindVertexArray(m_transform_vao);

		//Load our shader that will read data into our transform feedback buffer
		//Declare the names of all the varyings we will copy data into
		//Let Opengl know what are varyings are and how we want to pack them into buffers
		//Relink the program
		m_transform_feedback_program = LoadShaders(transform_feedback_shader_text);
		static const char* varyings[] = { "vPosition" };
		glTransformFeedbackVaryings(m_transform_feedback_program, 1, varyings, GL_INTERLEAVED_ATTRIBS);
		glLinkProgram(m_transform_feedback_program);

		//Generate a buffer to put our transform feedback data into, make it big enough to be able to hold all our data
		glGenBuffers(1, &m_feedback_buffer);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_feedback_buffer);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(float) * 3000, nullptr, GL_DYNAMIC_COPY);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
		glBindVertexArray(0);

		SetupDisplay();
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


		//Bind our program, bind out feedback buffer, begin our query of how many primitives are written to the transform feedback buffer
		//Start transform feedback and start drawing
		//End transform feedback, end our query
		glUseProgram(m_transform_feedback_program);
		glBindVertexArray(m_transform_vao);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_feedback_buffer);
		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_queries);
		glBeginTransformFeedback(GL_TRIANGLES);
		glDrawArrays(GL_TRIANGLES, 0, 9);
		glEndTransformFeedback();
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

		//Call glFinish just to make sure our data is ready for the example
		glFinish();

		//Get our result to find out how many primitives were written into our transform feedback buffer
		//Use our result in our next draw call
		glGetQueryObjectuiv(m_queries, GL_QUERY_RESULT, &m_result);
		Display(m_result);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Text("Primitives written: %d", m_result);
		ImGui::End();
	}
	void SetupDisplay() {
		m_program = LoadShaders(shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 2.0f, 15.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.1, 1000.0);

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);
		glVertexAttribBinding(0, 0);
		glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayVertexBuffer(m_vao, 0, m_feedback_buffer, 0, sizeof(float) * 3);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
	}
	void Display(int primitives) {
		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_feedback_buffer);
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glDrawArrays(GL_TRIANGLES, 0, primitives * 3);
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
#endif //TRANSFORM_FEEDBACK_QUERIES