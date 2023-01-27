#include "Defines.h"
#ifdef QUINTIC_BEZIER
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

	glm::vec4 m_control_points[5];

	std::vector<glm::vec4> m_points;
	glm::vec4 m_point;
	glm::vec4 m_anchors_0[4];
	glm::vec4 m_anchors_1[3];
	glm::vec4 m_anchors_2[2];

	float m_step;

	GLuint VBO;
	GLuint VAO;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		m_step(0.01f),
		VAO(0),
		VBO(0),
		m_program(0),
		m_point{},
		m_anchors_0{},
		m_anchors_1{},
		m_anchors_2{}
	{
		glm::vec4 a(-1.0f, -1.0f, 0.5f, 1.0f);
		glm::vec4 b(-0.5f, 1.0f, 0.5f, 1.0f);
		glm::vec4 c(0.11f, -1.0f, 0.5f, 1.0f);
		glm::vec4 d(0.84f, -0.7f, 0.5f, 1.0f);
		glm::vec4 e(1.0f, 1.0f, 0.5f, 1.0f);
		m_control_points[0] = a;
		m_control_points[1] = b;
		m_control_points[2] = c;
		m_control_points[3] = d;
		m_control_points[4] = e;
	}

	void OnInit(Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glEnable(GL_LINE_SMOOTH);
		glPointSize(10.0);
		m_program = LoadShaders(shader_text);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		//Gen curve
		m_points.clear();
		f64 t = 0.0;

		while (t < 1.0) {
			if (t + m_step > 1.0) {
				t += 1.0 - t;
			}
			glm::vec4 f = glm::mix(m_control_points[0], m_control_points[1], t);
			glm::vec4 g = glm::mix(m_control_points[1], m_control_points[2], t);
			glm::vec4 h = glm::mix(m_control_points[2], m_control_points[3], t);
			glm::vec4 i = glm::mix(m_control_points[3], m_control_points[4], t);

			glm::vec4 j = glm::mix(f, g, t);
			glm::vec4 k = glm::mix(g, h, t);
			glm::vec4 l = glm::mix(h, i, t);

			glm::vec4 m = glm::mix(j, k, t);
			glm::vec4 n = glm::mix(k, l, t);

			glm::vec4 point = glm::mix(m, n, t);

			m_points.push_back(point);
			t += m_step;
		}

		//Get point and anchors
		t = ((sin(m_time * 0.25) + 1.0) * 0.5);
		glm::vec4 f = glm::mix(m_control_points[0], m_control_points[1], t);
		glm::vec4 g = glm::mix(m_control_points[1], m_control_points[2], t);
		glm::vec4 h = glm::mix(m_control_points[2], m_control_points[3], t);
		glm::vec4 i = glm::mix(m_control_points[3], m_control_points[4], t);

		glm::vec4 j = glm::mix(f, g, t);
		glm::vec4 k = glm::mix(g, h, t);
		glm::vec4 l = glm::mix(h, i, t);

		glm::vec4 m = glm::mix(j, k, t);
		glm::vec4 n = glm::mix(k, l, t);

		m_point = glm::mix(m, n, t);

		m_anchors_0[0] = f;
		m_anchors_0[1] = g;
		m_anchors_0[2] = h;
		m_anchors_0[3] = i;

		m_anchors_1[0] = j;
		m_anchors_1[1] = k;
		m_anchors_1[2] = l;

		m_anchors_2[0] = m;
		m_anchors_2[1] = n;
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program);

		static const float white[] = { 1.0, 1.0, 1.0, 1.0 };
		static const float red[] = { 1.0, 0.0, 0.0, 1.0 };
		static const float green[] = { 0.0, 1.0, 0.0, 1.0 };
		static const float blue[] = { 0.0, 0.0, 1.0, 1.0 };
		static const float yellow[] = { 1.0, 1.0, 0.0, 1.0 };

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glEnableVertexAttribArray(0);

		//Draw control points
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_control_points), m_control_points, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glVertexAttrib4fv(1, white);
		glDrawArrays(GL_LINE_STRIP, 0, 5);

		//Draw Curve
		glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(float) * 4, &m_points[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glVertexAttrib4fv(1, red);
		glDrawArrays(GL_LINE_STRIP, 0, static_cast<i32>(m_points.size()));

		//Draw Curve Point
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4, &m_point, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glVertexAttrib4fv(1, green);
		glDrawArrays(GL_POINTS, 0, 1);

		//Draw Anchors 0
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, &m_anchors_0[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glVertexAttrib4fv(1, yellow);
		glDrawArrays(GL_LINE_STRIP, 0, 4);

		//Draw Anchors 1
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 3, &m_anchors_1[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glVertexAttrib4fv(1, green);
		glDrawArrays(GL_LINE_STRIP, 0, 3);

		//Draw Anchors 2
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, &m_anchors_2[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glVertexAttrib4fv(1, blue);
		glDrawArrays(GL_LINE_STRIP, 0, 2);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::SliderFloat2("Control Point 1", (float*)&m_control_points[0], -1.0, 1.0);
		ImGui::SliderFloat2("Control Point 2", (float*)&m_control_points[1], -1.0, 1.0);
		ImGui::SliderFloat2("Control Point 3", (float*)&m_control_points[2], -1.0, 1.0);
		ImGui::SliderFloat2("Control Point 4", (float*)&m_control_points[3], -1.0, 1.0);
		ImGui::SliderFloat2("Control Point 5", (float*)&m_control_points[4], -1.0, 1.0);
		ImGui::SliderFloat("Step", &m_step, 0.001f, 0.1f);
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
#endif //CUBIC_BEZIER
