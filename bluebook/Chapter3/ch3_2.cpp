#include "Current_Project.h"
#ifdef CH3_2
#include "System.h"

GLuint compile_shaders();

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_vertex_array_object;

	Application()
		:m_clear_color{ 0.0f, 0.2f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		m_program(NULL),
		m_vertex_array_object(NULL)
	{}

	~Application() {
		glDeleteVertexArrays(1, &m_vertex_array_object);
		glDeleteProgram(m_program);
	}

	void OnInit(Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");
		m_program = compile_shaders();
		glCreateVertexArrays(1, &m_vertex_array_object);
		glBindVertexArray(m_vertex_array_object);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		m_clear_color[0] = static_cast<float>(sin(m_time) * 0.5 + 0.5);
		m_clear_color[1] = static_cast<float>(cos(m_time) * 0.5 + 0.5);
	}
	void OnDraw() {
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_PATCHES, 0, 3);
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
		144,						//framelimit
		"resources/Icon.bmp" //icon path
};

MAIN(config)

GLuint compile_shaders() {
	GLuint vertex_shader;
	GLuint tess_control_shader;
	GLuint tess_eval_shader;
	GLuint fragment_shader;
	GLuint program;

	static const GLchar* vertex_shader_source = R"(
#version 450 core

void main() 
{
	const vec4 vertices[3] = vec4[3] (vec4(0.25, -0.25, 0.5, 1.0),
									  vec4(-0.25, -0.25, 0.5, 1.0),
									  vec4(0.25, 0.25, 0.5, 1.0));

	gl_Position = vertices[gl_VertexID];
}
)";

	static const GLchar* tess_control_shader_source = R"(
#version 450 core

layout (vertices = 3) out;

void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelInner[0] = 5.0;
		gl_TessLevelOuter[0] = 5.0;
		gl_TessLevelOuter[1] = 5.0;
		gl_TessLevelOuter[2] = 5.0;
	}
	
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

	static const GLchar* tess_eval_shader_source = R"(
#version 450 core

layout (triangles, equal_spacing, cw) in;

void main()
{
	gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position +
					gl_TessCoord.y * gl_in[1].gl_Position +
					gl_TessCoord.z * gl_in[2].gl_Position);
}
)";

	static const GLchar* fragment_shader_source = R"(
#version 450 core



out vec4 color;

void main() 
{
	color = vec4(0.0, 0.5, 1.0, 1.0);
}
)";

	//compile vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	//compile tess control shader
	tess_control_shader = glCreateShader(GL_TESS_CONTROL_SHADER);
	glShaderSource(tess_control_shader, 1, &tess_control_shader_source, NULL);
	glCompileShader(tess_control_shader);

	//compile tess eval shader
	tess_eval_shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
	glShaderSource(tess_eval_shader, 1, &tess_eval_shader_source, NULL);
	glCompileShader(tess_eval_shader);

	//compile fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	//attach and link shaders
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, tess_control_shader);
	glAttachShader(program, tess_eval_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	//clean up
	glDeleteShader(vertex_shader);
	glDeleteShader(tess_control_shader);
	glDeleteShader(tess_eval_shader);
	glDeleteShader(fragment_shader);

	return program;
}
#endif //TEST