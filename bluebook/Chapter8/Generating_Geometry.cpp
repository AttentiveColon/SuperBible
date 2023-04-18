#include "Defines.h"
#ifdef GENERATING_GEOMETRY
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

void main()
{
	gl_Position = vec4(position, 1.0);
}
)";

static const GLchar* geometry_shader_source = R"(
#version 450 core

layout (triangles) in;
layout (triangle_strip) out;
layout (max_vertices = 12) out;

layout (location = 12)
uniform mat4 mvp;

layout (location = 13)
uniform float stretch_factor;

out GS_OUT
{
	vec4 color;
} gs_out;

void make_face(vec3 a, vec3 b, vec3 c)
{
	vec3 face_normal = normalize(cross(c - a, c - b));
	vec4 face_color = vec4(1.0, 0.2, 0.4, 1.0) * vec4(face_normal, 1.0);
	gl_Position = mvp * vec4(a, 1.0);
	gs_out.color = face_color;
	EmitVertex();

	gl_Position = mvp * vec4(b, 1.0);
	gs_out.color = face_color;
	EmitVertex();
	
	gl_Position = mvp * vec4(c, 1.0);
	gs_out.color = face_color;
	EmitVertex();

	EndPrimitive();
}

void main()
{
	vec3 a = gl_in[0].gl_Position.xyz;
	vec3 b = gl_in[1].gl_Position.xyz;
	vec3 c = gl_in[2].gl_Position.xyz;

	vec3 d = (a + b) * stretch_factor;
	vec3 e = (b + c) * stretch_factor;
	vec3 f = (c + a) * stretch_factor;

	a *= (2.0 - stretch_factor);
	b *= (2.0 - stretch_factor);
	c *= (2.0 - stretch_factor);

	make_face(a, d, f);
	make_face(d, b, e);
	make_face(e, c, f);
	make_face(d, e, f);
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

in GS_OUT
{
	vec4 color;
} fs_in;

void main()
{
	color = fs_in.color;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_GEOMETRY_SHADER, geometry_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};




struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;



	float m_stretch_factor = 0.667f;

	SB::Camera m_camera;
	bool m_input_mode = false;


	bool m_wireframe = false;
	ObjMesh m_mesh;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glFrontFace(GL_CCW);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);


		m_program = LoadShaders(shader_text);

		m_mesh.Load_OBJ("./resources/cube.obj");

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		m_stretch_factor = sin(m_time) * 0.25 + 0.5;

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
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		if (m_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glUseProgram(m_program);
		glUniformMatrix4fv(12, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));
		glUniform1f(13, m_stretch_factor);
		m_mesh.OnDraw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
		ImGui::SliderFloat("StretchFactor", &m_stretch_factor, 0.0f, 2.0f);
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
#endif //GENERATING_GEOMETRY
