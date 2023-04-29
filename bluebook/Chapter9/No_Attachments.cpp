#include "Defines.h"
#ifdef NO_ATTACHMENTS
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 460 core


void main()
{
	const vec2 vertices[] = 
	{
		vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0),
		vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(1.0, -1.0)
	};
	gl_Position = vec4(vertices[gl_VertexID], 0.5, 1.0);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 460 core

out vec4 color;

layout (binding = 0, offset = 0)
uniform atomic_uint u_counter;


void main()
{
	uint x = uint(gl_FragCoord.x);
	if (x == 1)
	{
		atomicCounterIncrement(u_counter);
	}
	color = vec4(1.0, 0.0, 0.0, 1.0);
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

	uint m_count = 0;
	GLuint m_program, m_vao, m_counter, m_fbo;



	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);
		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);
		glGenBuffers(1, &m_counter);
		
		//Create fbo with no attachments
		glGenFramebuffers(1, &m_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

		//set custom fbo parameters for width and height
		glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 10000);
		glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, 10000);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		//Bind our fbo and create a viewport to our 10,000 x 10,000 scene
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glViewport(0, 0, 10000, 10000);
		RenderScene();

		//Unbind out fbo and view the Atomic value in ImGui to verify 10,000 executions of the atomic increment
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::Text("Atomic Value: %d", m_count);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::End();
	}

	void RenderScene() {
		glUseProgram(m_program);

		const GLuint zero = 0;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_counter);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_counter);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_counter);
		GLuint* count = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
		m_count = *count;
		*count = 0;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
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
#endif //NO_ATTACHMENTS
