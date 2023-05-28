#include "Defines.h"
#ifdef INDIRECT_MATERIAL
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include <chrono>
#include <omp.h>

static const GLchar* vs_source = R"(
#version 450 core

#extension GL_ARB_shader_draw_parameters : require

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (std140, binding = 0) uniform FRAME_DAT
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
};

layout (std140, binding = 0) readonly buffer OBJECT_TRANSFORMS
{
	mat4 model_matrix[];
};

layout (location = 0) uniform vec3 light_pos;
layout (location = 1) uniform vec3 cam_pos;

out VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
	flat int material_id;
} vs_out;

void main()
{
	mat4 model = model_matrix[gl_DrawIDARB];
	vec4 P = model * vec4(position, 1.0);

	vs_out.N = mat3(model) * normal;
	vs_out.L = light_pos - P.xyz;
	vs_out.V = cam_pos - P.xyz;
	vs_out.uv = uv;
	vs_out.material_id = gl_BaseInstanceARB;

	gl_Position = proj * view * P;
}
)";

static const GLchar* fs_source = R"(
#version 450 core

layout (location = 0) out vec4 color;

struct MaterialProperties
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

layout (binding = 2) uniform MATERIALS
{
	MaterialProperties material[100];
};

in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
	flat int material_id;
} fs_in;

void main()
{
	vec3 ambient = material[fs_in.material_id].ambient.rgb;
	vec3 specular_albedo = material[fs_in.material_id].specular.rgb;
	vec3 diffuse_albedo = material[fs_in.material_id].diffuse.rgb;
	float specular_power = material[fs_in.material_id].specular.a;

	vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);
    vec3 H = normalize(L + V);

	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
	vec3 specular = pow(max(dot(N, H), 0.0), specular_power * 1.0) * specular_albedo;

	color = vec4(ambient + specular + diffuse, 1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vs_source, NULL},
	{GL_FRAGMENT_SHADER, fs_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct DrawArraysIndirectCommand
{
	GLuint  count;
	GLuint  primCount;
	GLuint  first;
	GLuint  baseInstance;
};

struct MaterialProperties
{
	glm::vec4		ambient;
	glm::vec4		diffuse;
	glm::vec3		specular;
	float           specular_power;
};

struct FrameUniforms
{
	glm::mat4 view_matrix;
	glm::mat4 proj_matrix;
	glm::mat4 viewproj_matrix;
};

#define NUM_DRAWS 1638
#define NUM_MATERIALS 100

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_indirect_draw_buffer;
	GLuint m_transform_buffer;
	GLuint m_frame_uniforms_buffer;
	GLuint m_material_buffer;
	
	ObjMesh m_object;

	SB::Camera m_camera;
	bool m_input_mode = false;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);
		m_object.Load_OBJ("./resources/cube.obj");
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 8.0f, 15.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_indirect_draw_buffer = CreateIndirectBuffer();
		m_transform_buffer = CreateTransformBuffer();
		m_frame_uniforms_buffer = CreateFrameUniformsBuffer();
		m_material_buffer = CreateMaterialBuffer();

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_CULL_FACE);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (m_input_mode) {
			m_camera.OnUpdate(input, 3.0f, 0.2f, dt);
		}
		if (input.Pressed(GLFW_KEY_LEFT_CONTROL)) {
			m_input_mode = !m_input_mode;
			input.SetRawMouseMode(window.GetHandle(), m_input_mode);
		}
	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program);
		glBindVertexArray(m_object.m_vao);

		//Bind Frame Uniforms Buffer
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_frame_uniforms_buffer);
		FrameUniforms* pUniforms = (FrameUniforms*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(FrameUniforms), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		pUniforms->proj_matrix = m_camera.m_proj;
		pUniforms->view_matrix = m_camera.m_view;
		pUniforms->viewproj_matrix = m_camera.ViewProj();
		glUnmapBuffer(GL_UNIFORM_BUFFER);

		//Bind Model Matrix Buffer
		glm::mat4* pModelMatrices = (glm::mat4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_DRAWS * sizeof(glm::mat4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_transform_buffer);

		float f = m_time * 0.01f;
		for (int i = 0; i < NUM_DRAWS; i++)
		{
			glm::mat4 m = glm::translate(glm::vec3(sinf(f * 7.3f) * 70.0f, sinf(f * 3.7f + 2.0f) * 70.0f, sinf(f * 2.9f + 8.0f) * 70.0f)) *
				glm::rotate(f * 330.0f, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(f * 490.0f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(f * 250.0f, glm::vec3(0.0f, 0.0f, 1.0f));
			pModelMatrices[i] = m;
			f += 3.1f;
		}

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glUniform3fv(0, 1, glm::value_ptr(glm::vec3(10.0f, 10.0f, 10.0f))); //Light Position
		glUniform3fv(1, 1, glm::value_ptr(m_camera.Eye())); //Camera Position

		glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_material_buffer);

		
		glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, NUM_DRAWS, 0);

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::End();
	}
	GLuint CreateIndirectBuffer() {
		GLuint indirect_draw_buffer;
		glGenBuffers(1, &indirect_draw_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_draw_buffer);
		glBufferStorage(GL_DRAW_INDIRECT_BUFFER,
			NUM_DRAWS * sizeof(DrawArraysIndirectCommand),
			nullptr,
			GL_MAP_WRITE_BIT);

		DrawArraysIndirectCommand* cmd = (DrawArraysIndirectCommand*)
			glMapBufferRange(GL_DRAW_INDIRECT_BUFFER,
				0,
				NUM_DRAWS * sizeof(DrawArraysIndirectCommand),
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		
		for (int i = 0; i < NUM_DRAWS; i++)
		{
			cmd[i].first = 0;
			cmd[i].count = m_object.m_count;
			cmd[i].primCount = 1;
			cmd[i].baseInstance = i % NUM_MATERIALS;
		}

		glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);
		return indirect_draw_buffer;
	}
	GLuint CreateTransformBuffer() {
		GLuint transform_buffer;
		glGenBuffers(1, &transform_buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, transform_buffer);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, NUM_DRAWS * sizeof(glm::mat4), nullptr, GL_MAP_WRITE_BIT);

		return transform_buffer;
	}
	GLuint CreateFrameUniformsBuffer() {
		GLuint frame_uniforms_buffer;
		glGenBuffers(1, &frame_uniforms_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, frame_uniforms_buffer);
		glBufferStorage(GL_UNIFORM_BUFFER, sizeof(FrameUniforms), nullptr, GL_MAP_WRITE_BIT);

		return frame_uniforms_buffer;
	}
	GLuint CreateMaterialBuffer() {
		GLuint material_buffer;
		glGenBuffers(1, &material_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, material_buffer);
		glBufferStorage(GL_UNIFORM_BUFFER, NUM_MATERIALS * sizeof(MaterialProperties), nullptr, GL_MAP_WRITE_BIT);

		MaterialProperties* pMaterial = (MaterialProperties*)
			glMapBufferRange(GL_UNIFORM_BUFFER,
				0,
				NUM_MATERIALS * sizeof(MaterialProperties),
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

		for (int i = 0; i < NUM_MATERIALS; i++)
		{
			float f = float(i) / float(NUM_MATERIALS);
			pMaterial[i].ambient = (glm::vec4(sinf(f * 3.7f), sinf(f * 5.7f + 3.0f), sinf(f * 4.3f + 2.0f), 1.0f) + glm::vec4(1.0f, 1.0f, 2.0f, 0.0f)) * 0.1f;
			pMaterial[i].diffuse = (glm::vec4(sinf(f * 9.9f + 6.0f), sinf(f * 3.1f + 2.5f), sinf(f * 7.2f + 9.0f), 1.0f) + glm::vec4(1.0f, 2.0f, 2.0f, 0.0f)) * 0.4f;
			pMaterial[i].specular = (glm::vec3(sinf(f * 1.6f + 4.0f), sinf(f * 0.8f + 2.7f), sinf(f * 5.2f + 8.0f)) + glm::vec3(19.0f, 19.0f, 19.0f)) * 0.6f;
			pMaterial[i].specular_power = 200.0f + sinf(f) * 50.0f;
		}

		glUnmapBuffer(GL_UNIFORM_BUFFER);

		return material_buffer;
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
#endif //INDIRECT_MATERIAL