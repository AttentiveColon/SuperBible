#include "Defines.h"
#ifdef SPLINES
#include "System.h"

#include <vector>

#include "glm/glm.hpp"
#include "glm/common.hpp"

using namespace glm;

static const float white[] = { 1.0, 1.0, 1.0, 1.0 };
static const float red[] = { 1.0, 0.0, 0.0, 1.0 };
static const float green[] = { 0.0, 1.0, 0.0, 1.0 };
static const float blue[] = { 0.0, 0.0, 1.0, 1.0 };
static const float yellow[] = { 1.0, 1.0, 0.0, 1.0 };

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

vec4 Quadratic_Bezier(vec2 cp1, vec2 cp2, vec2 cp3, f32 t) {
	vec2 a = mix(cp1, cp2, t);
	vec2 b = mix(cp2, cp3, t);

	vec2 point = mix(a, b, t);
	return vec4(point, 0.0f, 1.0f);
}

vec4 Cubic_Bezier(vec2 cp1, vec2 cp2, vec2 cp3, vec2 cp4, f32 t) {
	vec2 a = mix(cp1, cp2, t);
	vec2 b = mix(cp2, cp3, t);
	vec2 c = mix(cp3, cp4, t);

	return Quadratic_Bezier(a, b, c, t);
}

vec4 Quintic_Bezier(vec2 cp1, vec2 cp2, vec2 cp3, vec2 cp4, vec2 cp5, f32 t) {
	vec2 a = mix(cp1, cp2, t);
	vec2 b = mix(cp2, cp3, t);
	vec2 c = mix(cp3, cp4, t);
	vec2 d = mix(cp4, cp5, t);

	return Cubic_Bezier(a, b, c, d, t);
}

struct Quintic_Bezier_Anchors {
	std::vector<vec4> m_anchors_0;
	std::vector<vec4> m_anchors_1;
	std::vector<vec4> m_anchors_2;
	vec4 m_point;
};

struct Quintic_Bezier_Obj {
	Quintic_Bezier_Obj() :m_point{} {
		m_cp.push_back(vec4(0.0, 0.0, 0.0, 1.0));
		m_cp.push_back(vec4(0.0, 0.0, 0.0, 1.0));
		m_cp.push_back(vec4(0.0, 0.0, 0.0, 1.0));
		m_cp.push_back(vec4(0.0, 0.0, 0.0, 1.0));
		m_cp.push_back(vec4(0.0, 0.0, 0.0, 1.0));
	}
	std::vector<vec4> m_cp;
	vec4 m_point;
	std::vector<vec4> m_curve_points;
	Quintic_Bezier_Anchors m_anchors;

	void Update(f64 time) {
		Gen_Curve();
		Get_Anchors(time);
	}

	void Gen_Curve() {
		//Gen curve
		m_curve_points.clear();
		f64 t = 0.0;
		f64 step = 0.01;

		while (t < 1.0) {
			if (t + step > 1.0) {
				t += 1.0 - t;
			}
			vec4 point = Quintic_Bezier(
				m_cp[0],
				m_cp[1],
				m_cp[2],
				m_cp[3],
				m_cp[4],
				t
			);

			m_curve_points.push_back(point);
			t += step;
		}
	}

	void Get_Anchors(f32 time) {
		f64 t = ((sin(time * 0.25) + 1.0) * 0.5);
		Quintic_Bezier_Anchors anchors;
		vec2 a = mix(m_cp[0], m_cp[1], t);
		vec2 b = mix(m_cp[1], m_cp[2], t);
		vec2 c = mix(m_cp[2], m_cp[3], t);
		vec2 d = mix(m_cp[3], m_cp[4], t);

		anchors.m_anchors_0.push_back(vec4(a, 0.0, 1.0));
		anchors.m_anchors_0.push_back(vec4(b, 0.0, 1.0));
		anchors.m_anchors_0.push_back(vec4(c, 0.0, 1.0));
		anchors.m_anchors_0.push_back(vec4(d, 0.0, 1.0));

		vec2 e = mix(a, b, t);
		vec2 f = mix(b, c, t);
		vec2 g = mix(c, d, t);

		anchors.m_anchors_1.push_back(vec4(e, 0.0, 1.0));
		anchors.m_anchors_1.push_back(vec4(f, 0.0, 1.0));
		anchors.m_anchors_1.push_back(vec4(g, 0.0, 1.0));

		vec2 h = mix(e, f, t);
		vec2 i = mix(f, g, t);

		anchors.m_anchors_2.push_back(vec4(h, 0.0, 1.0));
		anchors.m_anchors_2.push_back(vec4(i, 0.0, 1.0));

		vec2 point = mix(h, i, t);
		anchors.m_point = vec4(point, 0.0, 1.0);

		m_anchors = anchors;
	}

	void Draw(bool draw_anchors, bool draw_control_points) {
		if (draw_control_points) Draw_Control_Points();
		Draw_Curve();
		Draw_Anchors(m_anchors, draw_anchors);
	}

	void Draw_Control_Points() {
		//Draw control points
		glBufferData(GL_ARRAY_BUFFER, m_cp.size() * sizeof(float) * 4, &m_cp[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glVertexAttrib4fv(1, white);
		glDrawArrays(GL_LINE_STRIP, 0, 5);
	}

	void Draw_Curve() {
		//Draw Curve
		glBufferData(GL_ARRAY_BUFFER, m_curve_points.size() * sizeof(float) * 4, &m_curve_points[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glVertexAttrib4fv(1, red);
		glDrawArrays(GL_LINE_STRIP, 0, static_cast<i32>(m_curve_points.size()));
	}

	void Draw_Anchors(Quintic_Bezier_Anchors anchors, bool draw_anchors) {
		//Draw Curve Point
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4, &anchors.m_point, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glVertexAttrib4fv(1, green);
		glDrawArrays(GL_POINTS, 0, 1);

		if (draw_anchors)
		{
			//Draw Anchors 0
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, &anchors.m_anchors_0[0], GL_STATIC_DRAW);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glVertexAttrib4fv(1, yellow);
			glDrawArrays(GL_LINE_STRIP, 0, 4);

			//Draw Anchors 1
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 3, &anchors.m_anchors_1[0], GL_STATIC_DRAW);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glVertexAttrib4fv(1, green);
			glDrawArrays(GL_LINE_STRIP, 0, 3);

			//Draw Anchors 2
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, &anchors.m_anchors_2[0], GL_STATIC_DRAW);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glVertexAttrib4fv(1, blue);
			glDrawArrays(GL_LINE_STRIP, 0, 2);
		}
		
	}
};


struct Quintic_Bezier_Group {
	Quintic_Bezier_Group() :draw_anchors(true), draw_control_points(true) {}

	std::vector<Quintic_Bezier_Obj> m_objects;
	bool draw_anchors;
	bool draw_control_points;

	void Update(f64 time) {
		for (int i = 0; i < m_objects.size(); ++i) {
			m_objects[i].Update(time);
		}
	}

	void Draw() {
		for (int i = 0; i < m_objects.size(); ++i) {
			m_objects[i].Draw(draw_anchors, draw_control_points);
		}
	}

	void Draw_Gui() {
		for (int i = 0; i < m_objects.size(); ++i) {
			for (int j = 0; j < m_objects[i].m_cp.size(); ++j) {
				char buf[11];
				sprintf_s(buf, "OBJ %d CP %d", i, j);
				ImGui::SliderFloat2(buf, (float*)&m_objects[i].m_cp[j], -1.0, 1.0);
			}
		}
	}

	void Add() {
		m_objects.push_back(Quintic_Bezier_Obj());
	}
};




struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	GLuint m_program;

	//Quintic_Bezier_Obj m_quint;
	Quintic_Bezier_Group m_quints;


	GLuint VBO;
	GLuint VAO;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0),
		VAO(0),
		VBO(0),
		m_quints(),
		m_program(0)
	{}

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

		if (input.Pressed(GLFW_KEY_SPACE)) {
			m_quints.Add();
		}
		
		m_quints.Update(m_time);
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glUseProgram(m_program);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glEnableVertexAttribArray(0);

		m_quints.Draw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Draw Anchors", &m_quints.draw_anchors);
		ImGui::Checkbox("Draw Control Points", &m_quints.draw_control_points);
		m_quints.Draw_Gui();
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
#endif //SPLINES
