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
    const vec3 positions[] = vec3[] (vec3(-0.5, -0.5, 0.0), vec3(0.0, 0.5, 0.0), vec3(0.5, -0.5, 0.0));
	vPosition = positions[gl_VertexID % 3];
}
)";

static ShaderText transform_feedback_shader_text[] = {
	{GL_VERTEX_SHADER, transform_feedback_vertex_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_transform_feedback_program;
	GLuint m_queries;
	GLuint m_feedback_buffer;
	GLuint m_result;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glGenQueries(1, &m_queries);

		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

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
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);


		//Bind our program, bind out feedback buffer, begin our query of how many primitives are written to the transform feedback buffer
		//Start transform feedback and start drawing
		//End transform feedback, end our query
		glUseProgram(m_transform_feedback_program);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_feedback_buffer);
		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_queries);
		glBeginTransformFeedback(GL_TRIANGLES);
		glDrawArrays(GL_TRIANGLES, 0, 303);
		glEndTransformFeedback();
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

		//Call glFinish just to make sure our data is ready for the example
		glFinish();

		//Get our result to find out how many primitives were written into our transform feedback buffer
		glGetQueryObjectuiv(m_queries, GL_QUERY_RESULT, &m_result);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Text("Primitives written: %d", m_result);
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
#endif //TRANSFORM_FEEDBACK_QUERIES