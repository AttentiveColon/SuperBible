#include "Defines.h"
#ifdef BEZIER_CURVES
#include "System.h"

#include <vector>

#include "glm/vec4.hpp"
#include "glm/common.hpp"

static const GLchar* vss = R"(
#version 450 core

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 color;

out vec4 vs_color;

void main()
{
	gl_Position = pos;
	vs_color = color;
}
)";

static const GLchar* fss = R"(
#version 450 core

in vec4 vs_color;
out vec4 color;

void main()
{
	color = vs_color;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vss, NULL},
	{GL_FRAGMENT_SHADER, fss, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	GLuint m_program;

	std::vector<glm::vec4> points;

	GLuint VBO;
	GLuint VBO2;
	GLuint VAO;
	GLuint VAO2;

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
		glGenVertexArrays(1, &VAO2);
		
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &VBO2);

		m_program = LoadShaders(shader_text);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		glm::vec4 a(controlpoints[0], controlpoints[1], 0.5, 1.0);
		glm::vec4 b(controlpoints[4], controlpoints[5], 0.5, 1.0);
		glm::vec4 c(controlpoints[8], controlpoints[9], 0.5, 1.0);
		glm::vec4 d(controlpoints[12], controlpoints[13], 0.5, 1.0);

		points.clear();

		f64 t = 0.0;
		f64 step = 0.01;

		while (t < 1.0) {
			glm::vec4 e = glm::mix(a, b, t);
			glm::vec4 f = glm::mix(b, c, t);
			glm::vec4 g = glm::mix(c, d, t);

			glm::vec4 h = glm::mix(e, f, t);
			glm::vec4 i = glm::mix(f, g, t);

			glm::vec4 point = glm::mix(h, i, t);

			points.push_back(point);
			t += step;
		}
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program);

		static const float white[] = { 1.0, 1.0, 1.0, 1.0 };
		static const float red[] = { 1.0, 0.0, 0.0, 1.0 };

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(controlpoints), controlpoints, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttrib4fv(1, white);
		glDrawArrays(GL_LINE_STRIP, 0, 4);
		glDisableVertexAttribArray(0);

		glBindVertexArray(VAO2);
		glBindBuffer(GL_ARRAY_BUFFER, VBO2);
		glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float) * 4, &points[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttrib4fv(1, red);
		glDrawArrays(GL_LINE_STRIP, 0, points.size());
		glDisableVertexAttribArray(0);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::SliderFloat2("Control Point 1", (float*)&controlpoints[0], -1.0, 1.0);
		ImGui::SliderFloat2("Control Point 2", (float*)&controlpoints[4], -1.0, 1.0);
		ImGui::SliderFloat2("Control Point 3", (float*)&controlpoints[8], -1.0, 1.0);
		ImGui::SliderFloat2("Control Point 4", (float*)&controlpoints[12], -1.0, 1.0);
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
