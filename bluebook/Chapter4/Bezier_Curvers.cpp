#include "Defines.h"
#ifdef BEZIER_CURVES
#include "System.h"

#include "glm/vec4.hpp"
#include "glm/common.hpp"

static const GLchar* vss = R"(
#version 450 core

layout (location = 0) in vec4 pos;


void main()
{
	gl_Position = pos;
}
)";

static const GLchar* fss = R"(
#version 450 core

out vec4 color;

void main()
{
	color = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static const GLchar* vss2 = R"(
#version 450 core

layout (location = 0) in vec4 pos;


void main()
{
	gl_Position = pos;
}
)";

static const GLchar* fss2 = R"(
#version 450 core

out vec4 color;

void main()
{
	color = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vss, NULL},
	{GL_FRAGMENT_SHADER, fss, NULL},
	{GL_NONE, NULL, NULL}
};

static ShaderText shader_text2[] = {
	{GL_VERTEX_SHADER, vss2, NULL},
	{GL_FRAGMENT_SHADER, fss2, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	GLuint m_program;
	GLuint m_program2;

	glm::vec4 point;

	GLuint VBO;
	GLuint VAO;

	float controlpoints[16];

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		controlpoints{-0.5, 0.0, 0.5, 1.0, -0.5, -0.5, 0.5, 1.0, 0.5, -0.5, 0.5, 1.0, 0.5, 0.0, 0.5, 1.0}
	{}

	void OnInit(Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glGenBuffers(1, &VBO);
		glPointSize(10.0);
		m_program = LoadShaders(shader_text);
		m_program2 = LoadShaders(shader_text2);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		f64 t = (sin(m_time * 0.5) + 1.0) * 0.5;

		glm::vec4 a(controlpoints[0], controlpoints[1], 0.5, 1.0);
		glm::vec4 b(controlpoints[4], controlpoints[5], 0.5, 1.0);
		glm::vec4 c(controlpoints[8], controlpoints[9], 0.5, 1.0);
		glm::vec4 d(controlpoints[12], controlpoints[13], 0.5, 1.0);


		glm::vec4 e = glm::mix(a, b, t);
		glm::vec4 f = glm::mix(b, c, t);
		glm::vec4 g = glm::mix(c, d, t);

		glm::vec4 h = glm::mix(e, f, t);
		glm::vec4 i = glm::mix(f, g, t);

		point = glm::mix(h, i, t);
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(controlpoints), controlpoints, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glUseProgram(m_program);
		glDrawArrays(GL_LINE_STRIP, 0, 4);

		glDisableVertexArrayAttrib(VBO, 0);

		glVertexAttrib4fv(0, (float*) & point);
		glUseProgram(m_program2);
		glDrawArrays(GL_POINTS, 0, 1);

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::SliderFloat4("Control Point 1", (float*)&controlpoints[0], -1.0, 1.0);
		ImGui::SliderFloat4("Control Point 2", (float*)&controlpoints[4], -1.0, 1.0);
		ImGui::SliderFloat4("Control Point 3", (float*)&controlpoints[8], -1.0, 1.0);
		ImGui::SliderFloat4("Control Point 4", (float*)&controlpoints[12], -1.0, 1.0);
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
#endif //TEST
