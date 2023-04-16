#include "Defines.h"
#ifdef DISPLACEMENT_MAPS
#include "System.h"
#include "Texture.h"
#include "Model.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

out VS_OUT
{
	vec2 tc;
} vs_out;

void main() 
{
	const vec4 vertices[] = vec4[](vec4(-0.5, 0.0, -0.5, 1.0),
								   vec4(0.5, 0.0, -0.5, 1.0),
								   vec4(-0.5, 0.0, 0.5, 1.0),
								   vec4(0.5, 0.0, 0.5, 1.0));

	int x = gl_InstanceID & 63;
	int y = gl_InstanceID >> 6;
	vec2 offs = vec2(x, y);
	
	vs_out.tc = (vertices[gl_VertexID].xz + offs + vec2(0.5)) / 64.0;
	gl_Position = vertices[gl_VertexID] + vec4(float(x - 32), 0.0, float(y - 32), 0.0);
}
)";

static const GLchar* tess_control_shader_source = R"(
#version 450 core

layout (vertices = 4) out;

in VS_OUT
{
    vec2 tc;
} tcs_in[];

out TCS_OUT
{
    vec2 tc;
} tcs_out[];

layout (location = 10)
uniform mat4 mvp_matrix;

void main(void)
{
    if (gl_InvocationID == 0)
    {
        vec4 p0 = mvp_matrix * gl_in[0].gl_Position;
        vec4 p1 = mvp_matrix * gl_in[1].gl_Position;
        vec4 p2 = mvp_matrix * gl_in[2].gl_Position;
        vec4 p3 = mvp_matrix * gl_in[3].gl_Position;
        p0 /= p0.w;
        p1 /= p1.w;
        p2 /= p2.w;
        p3 /= p3.w;
        if (p0.z <= 0.0 ||
            p1.z <= 0.0 ||
            p2.z <= 0.0 ||
            p3.z <= 0.0)
         {
              gl_TessLevelOuter[0] = 0.0;
              gl_TessLevelOuter[1] = 0.0;
              gl_TessLevelOuter[2] = 0.0;
              gl_TessLevelOuter[3] = 0.0;
         }
         else
         {
            float l0 = length(p2.xy - p0.xy) * 16.0 + 1.0;
            float l1 = length(p3.xy - p2.xy) * 16.0 + 1.0;
            float l2 = length(p3.xy - p1.xy) * 16.0 + 1.0;
            float l3 = length(p1.xy - p0.xy) * 16.0 + 1.0;
            gl_TessLevelOuter[0] = l0;
            gl_TessLevelOuter[1] = l1;
            gl_TessLevelOuter[2] = l2;
            gl_TessLevelOuter[3] = l3;
            gl_TessLevelInner[0] = min(l1, l3);
            gl_TessLevelInner[1] = min(l0, l2);
        }
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    tcs_out[gl_InvocationID].tc = tcs_in[gl_InvocationID].tc;
}
)";

static const GLchar* tess_evaluation_shader_source = R"(
#version 450 core

layout (quads, fractional_odd_spacing) in;

layout (binding = 0)
uniform sampler2D tex_displacement;

layout (location = 11)
uniform mat4 mv_matrix;
layout (location = 12)
uniform mat4 proj_matrix;
layout (location = 13)
uniform float dmap_depth;

in TCS_OUT
{
    vec2 tc;
} tes_in[];

out TES_OUT
{
    vec2 tc;

} tes_out;

void main(void)
{
    vec2 tc1 = mix(tes_in[0].tc, tes_in[1].tc, gl_TessCoord.x);
    vec2 tc2 = mix(tes_in[2].tc, tes_in[3].tc, gl_TessCoord.x);
    vec2 tc = mix(tc2, tc1, gl_TessCoord.y);

    vec4 p1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
    vec4 p2 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
    vec4 p = mix(p2, p1, gl_TessCoord.y);
    p.y += texture(tex_displacement, tc).r * dmap_depth;

    vec4 P_eye = mv_matrix * p;

    tes_out.tc = tc;

    gl_Position = proj_matrix * P_eye;
}
)";


static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

layout (binding = 1)
uniform sampler2D tex_color;

in TES_OUT
{
	vec2 tc;
} fs_in;

void main() 
{
	color = texture(tex_color, fs_in.tc);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_TESS_CONTROL_SHADER, tess_control_shader_source, NULL},
	{GL_TESS_EVALUATION_SHADER, tess_evaluation_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;

	GLuint m_vao;

	GLuint m_tex_displacment;
	GLuint m_tex_color;

	bool m_wireframe = false;

	SB::Camera m_camera;
	bool m_input_mode = false;

	float m_depth = 6.0f;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);

		m_program = LoadShaders(shader_text);

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		glPatchParameteri(GL_PATCH_VERTICES, 4);
		glEnable(GL_CULL_FACE);

		m_tex_displacment = Load_KTX("./resources/displacement.ktx");
		glActiveTexture(GL_TEXTURE1);
		m_tex_color = Load_KTX("./resources/face1.ktx");

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 5.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 0.001, 1000.0);

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
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program);

		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTextureUnit(0, m_tex_displacment);
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTextureUnit(1, m_tex_color);

		glUniformMatrix4fv(10, 1, GL_FALSE, glm::value_ptr(m_camera.m_viewproj));
		glUniformMatrix4fv(11, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(12, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform1f(13, m_depth);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		if (m_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		glDrawArraysInstanced(GL_PATCHES, 0, 4, 64 * 64);

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
		ImGui::DragFloat("Depth", &m_depth, 0.1f, 1.0f, 100.0f);
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
#endif //DISPLACEMENT_MAPS
