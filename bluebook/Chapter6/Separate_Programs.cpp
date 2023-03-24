#include "Defines.h"
#ifdef SEPARATE_PROGRAMS
#include "System.h"
#include "Texture.h"

#include <string>
#include <sstream>

static GLfloat size = 0.5f;

static const GLchar* vertex_shader_source = R"(
#version 450 core



layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

out vec2 vs_uv;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
	gl_Position = vec4(position, 1.0);
	vs_uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;
in vec2 vs_uv;

void main() 
{
	color = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static GLuint GenerateQuad() {
	GLuint vao, vertex_buffer;

	static const GLfloat vertex_array[] = {
	-1.0f, -1.0f, 0.5f, 0.0f, 1.0f,
	-1.0f, 1.0f, 0.5f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.5f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.5f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.5f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.5f, 1.0f, 1.0f,
	};

	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vertex_buffer);

	glNamedBufferStorage(vertex_buffer, sizeof(vertex_array), &vertex_array[0], 0);

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(vao, 0);

	glVertexArrayAttribBinding(vao, 1, 0);
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
	glEnableVertexArrayAttrib(vao, 1);

	glVertexArrayVertexBuffer(vao, 0, vertex_buffer, 0, sizeof(float) * 5);
	glBindVertexArray(0);

	return vao;
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	f32 m_index;

	GLuint m_program_pipeline;
	GLuint m_program_vertex;
	GLuint m_program_fragment;
	GLuint m_vao;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0),
		m_index(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		//Turn our raw string shaders into c-style strings just for this example


		//Create separable vertex and fragment shaders and then 
		// create a pipeline for them

		//Create pipeline to represent the collections of programs currently in use
		glGenProgramPipelines(1, &m_program_pipeline);
		

		//Create vertex shader, attach source, compile, then check status
		GLuint vs = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vs, 1, &vertex_shader_source, NULL);
		glCompileShader(vs);
		GetShaderCompilationStatus(vs);
		

		//create a program for our vertex stage to attach to
		m_program_vertex = glCreateProgram();
		glAttachShader(m_program_vertex, vs);

		//IMPORTANT - set the GL_PROGRAM_SEPARABLE flag THEN link
		glProgramParameteri(m_program_vertex, GL_PROGRAM_SEPARABLE, GL_TRUE);
		glLinkProgram(m_program_vertex);
		GetProgramLinkedStatus(m_program_vertex);

		//Now do the same for the fragment shader
		GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fs, 1, &fragment_shader_source, NULL);
		glCompileShader(fs);
		GetShaderCompilationStatus(fs);

		m_program_fragment = glCreateProgram();
		glAttachShader(m_program_fragment, fs);
		glProgramParameteri(m_program_fragment, GL_PROGRAM_SEPARABLE, GL_TRUE);
		glLinkProgram(m_program_fragment);
		GetProgramLinkedStatus(m_program_fragment);

		

		//Now bind the vertex and fragment programs to the pipeline
		glUseProgramStages(m_program_pipeline, GL_VERTEX_SHADER_BIT, m_program_vertex);
		glUseProgramStages(m_program_pipeline, GL_FRAGMENT_SHADER_BIT, m_program_fragment);

		//Now rather than calling glUseProgram, call glBindProgramPipeline() 
		

		//Gather pipeline resource info
		GLint outputs;
		glGetProgramInterfaceiv(m_program_vertex, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &outputs);
		static const GLenum props[] = { GL_TYPE, GL_LOCATION };

		GLint i;
		GLint params[2];
		GLchar name[64];
		const char* type_name;

		for (i = 0; i < outputs; i++) {
			glGetProgramResourceName(m_program_vertex, GL_PROGRAM_OUTPUT, i, sizeof(name), NULL, name);
			glGetProgramResourceiv(m_program_vertex, GL_PROGRAM_OUTPUT, i, 2, props, 2, NULL, params);
			//type_name = "wait";
			printf("Index %d: %d %s @ location %d.\n", i, params[0], name, params[1]);
		}

		
		glGetProgramInterfaceiv(m_program_fragment, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &outputs);

		

		for (i = 0; i < outputs; i++) {
			glGetProgramResourceName(m_program_fragment, GL_PROGRAM_OUTPUT, i, sizeof(name), NULL, name);
			glGetProgramResourceiv(m_program_fragment, GL_PROGRAM_OUTPUT, i, 2, props, 2, NULL, params);
			//type_name = "wait";
			printf("Index %d: %d %s @ location %d.\n", i, params[0], name, params[1]);
		}

		m_vao = GenerateQuad();

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);

		//glUseProgram(m_program);
		glUseProgram(0);
		glBindProgramPipeline(m_program_pipeline);
		glBindVertexArray(m_vao);
		
		//Bind size uniform
		//glUniform1f(2, size);
		//glProgramUniform1f(m_program_vertex, 2, size);
		glDrawArrays(GL_TRIANGLES, 0, 18);

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::SliderFloat("Size", &size, 0.1, 1.0);
		ImGui::SliderFloat("Index", &m_index, 0.0, 4.0);
		ImGui::End();
	}
};

SystemConf config = {
		900,					//width
		900,					//height
		300,					//Position x
		200,					//Position y
		"Application",			//window title
		false,					//windowed fullscreen
		false,					//vsync
		30,					//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //TEST
