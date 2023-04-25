#include "Defines.h"
#ifdef LAYERED_RENDERING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

out VS_OUT
{	
	vec3 normal;
	vec2 uv;
} vs_out;

void main()
{
	gl_Position = vec4(position, 1.0);
	vs_out.normal = normal;
	vs_out.uv = uv;
}
)";

static const GLchar* geometry_shader_source = R"(
#version 450 core

layout (invocations = 16, triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT
{	
	vec3 normal;
	vec2 uv;
} gs_in[];

out GS_OUT
{
	vec3 normal;
	vec2 uv;
	vec4 color;
} gs_out;

layout (binding = 0) uniform BLOCK
{
	mat4 proj_matrix;
	mat4 mv_matrix[16];
};

void main()
{
	const vec4 colors[16] = vec4[16](
		vec4(0.0, 0.0, 1.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0),
		vec4(0.0, 1.0, 1.0, 1.0), vec4(1.0, 0.0, 1.0, 1.0),
		vec4(1.0, 1.0, 0.0, 1.0), vec4(1.0, 1.0, 1.0, 1.0),
		vec4(0.0, 0.0, 0.5, 1.0), vec4(0.0, 0.5, 0.0, 1.0),
		vec4(0.0, 0.5, 0.5, 1.0), vec4(0.5, 0.0, 0.0, 1.0),
		vec4(0.5, 0.0, 0.5, 1.0), vec4(0.5, 0.5, 0.0, 1.0),
		vec4(0.5, 0.5, 0.5, 1.0), vec4(1.0, 0.5, 0.5, 1.0),
		vec4(0.5, 1.0, 0.5, 1.0), vec4(0.5, 0.5, 1.0, 1.0)
	);

	for (int i = 0; i < gl_in.length(); ++i)
	{
		gs_out.color = colors[gl_InvocationID];
		gs_out.uv = gs_in[i].uv;
		gs_out.normal = mat3(mv_matrix[gl_InvocationID]) * gs_in[i].normal;
		gl_Position = proj_matrix * mv_matrix[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}
)";


static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

in GS_OUT
{
	vec3 normal;
	vec2 uv;
	vec4 color;
} fs_in;

void main()
{
	//color = vec4(fs_in.uv.x, fs_in.uv.y, 0.0, 1.0);
	color = vec4(abs(fs_in.normal.z)) * fs_in.color;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_GEOMETRY_SHADER, geometry_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* vertex_shader_source2 = R"(
#version 450 core

out VS_OUT
{
	vec3 tc;
} vs_out;

void main()
{
	int vid = gl_VertexID;
	int iid = gl_InstanceID;
	float inst_x = float(iid % 4) / 2.0;
	float inst_y = float(iid >> 2) / 2.0;

	const vec4 vertices[] = vec4[] (
		vec4(-0.5, -0.5, 0.0, 1.0), vec4(0.5, -0.5, 0.0, 1.0),
		vec4(0.5, 0.5, 0.0, 1.0), vec4(-0.5, 0.5, 0.0, 1.0)
	);

	vec4 offs = vec4(inst_x - 0.75, inst_y - 0.75, 0.0, 0.0);

	gl_Position = vertices[vid] * vec4(0.25, 0.25, 1.0, 1.0) + offs;
	vs_out.tc = vec3(vertices[vid].xy + vec2(0.5), float(iid));
}
)";

static const GLchar* fragment_shader_source2 = R"(
#version 450 core

layout (binding = 0)
uniform sampler2DArray tex_array;

out vec4 color;

in VS_OUT
{
	vec3 tc;
} fs_in;

void main()
{
	color = texture(tex_array, fs_in.tc);
}
)";

static ShaderText shader_text2[] = {
	{GL_VERTEX_SHADER, vertex_shader_source2, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source2, NULL},
	{GL_NONE, NULL, NULL}
};


struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_program2;
	SB::Camera m_camera;
	bool m_input_mode = false;

	bool m_wireframe = false;
	ObjMesh m_cube;
	glm::vec3 m_cube_pos = glm::vec3(0.0f, 0.0f, 1.0f);

	GLuint m_vao;
	GLuint m_fbo, m_ubo;
	GLuint m_color_attachment, m_depth_attachment;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glFrontFace(GL_CCW);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glCullFace(GL_BACK);


		m_program = LoadShaders(shader_text);
		m_program2 = LoadShaders(shader_text2);

		m_cube.Load_OBJ("./resources/cube.obj");

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 1.1, 1000.0);

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		//Create UBO
		glGenBuffers(1, &m_ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, 17 * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);

		//Create color texture array
		glGenTextures(1, &m_color_attachment);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_color_attachment);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 512, 512, 16);

		//Create depth texture array
		glGenTextures(1, &m_depth_attachment);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_depth_attachment);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT32, 512, 512, 16);

		//Create framebuffer object and attach textures
		glGenFramebuffers(1, &m_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_color_attachment, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depth_attachment, 0);

		

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime() * 0.1;

		glm::mat4 mv_matrix = glm::translate(glm::vec3(0.0f, 0.0f, -4.0f)) * glm::rotate((float)m_time * 5.0f, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate((float)m_time * 30.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 proj_matrix = glm::perspective(50.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
		glm::mat4 mvp = proj_matrix * mv_matrix;

		struct UNIFORM_BUFFER
		{
			glm::mat4 proj_matrix;
			glm::mat4 mv_matrix[16];
		};

		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);
		UNIFORM_BUFFER* buffer = (UNIFORM_BUFFER*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(UNIFORM_BUFFER), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

		buffer->proj_matrix = glm::perspective(5.0f, 1.0f, 0.1f, 1000.0f);
		for (int i = 0; i < 16; ++i) {
			float fi = (float)(i + 12) / 16.0f;
			buffer->mv_matrix[i] = glm::translate(glm::vec3(0.0f, 0.0f, -4.0f)) * glm::rotate((float)m_time * 25.0f * fi, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate((float)m_time * 30.0f * fi, glm::vec3(1.0f, 0.0f, 0.0f));
		}
		glUnmapBuffer(GL_UNIFORM_BUFFER);

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
		static const GLfloat one = 1.0f;

		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		static const GLuint draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, draw_buffers);


		glViewport(0, 0, 512, 512);
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);


		if (m_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glUseProgram(m_program);
		m_cube.OnDraw();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);
		glUseProgram(m_program2);

		glViewport(0, 0, 1600, 900);
		glClearBufferfv(GL_COLOR, 0, m_clear_color);

		glBindTexture(GL_TEXTURE_2D_ARRAY, m_color_attachment);
		glDisable(GL_DEPTH_TEST);
		
		glBindVertexArray(m_vao);
		glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 16);

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
		ImGui::DragFloat3("Cube Position", glm::value_ptr(m_cube_pos), 0.1f, -5.0f, 5.0f);
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
#endif //LAYERED_RENDERING
