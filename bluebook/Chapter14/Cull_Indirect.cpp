#include "Defines.h"
#ifdef CULL_INDIRECT
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include <chrono>
#include <omp.h>

static const GLchar* compute_source = R"(
#version 450 core

layout (local_size_x = 16) in;

struct CandidateDraw
{
    vec3 sphereCenter;
    float sphereRadius;
    uint firstVertex;
    uint vertexCount;
};

struct DrawArraysIndirectCommand
{
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint baseInstance;
};

layout (binding = 0, std430) buffer CandidateDraws
{
    CandidateDraw draw[];
};

layout (binding = 1, std430) writeonly buffer OutputDraws
{
    DrawArraysIndirectCommand command[];
};

layout (binding = 0, std140) uniform MODEL_MATRIX_BLOCK
{
    mat4    model_matrix[1024];
};

layout (binding = 1, std140) uniform TRANSFORM_BLOCK
{
    mat4    view_matrix;
    mat4    proj_matrix;
    mat4    view_proj_matrix;
};

layout (binding = 0, offset = 0) uniform atomic_uint commandCounter;

void main(void)
{
    const CandidateDraw thisDraw = draw[gl_GlobalInvocationID.x];
    const mat4 thisModelMatrix = model_matrix[gl_GlobalInvocationID.x];

    vec4 position = view_proj_matrix * thisModelMatrix * vec4(thisDraw.sphereCenter, 1.0);

    if ((abs(position.x) - thisDraw.sphereRadius) < (position.w * 1.0) &&
        (abs(position.y) - thisDraw.sphereRadius) < (position.w * 1.0))
    {
        uint outDrawIndex = atomicCounterIncrement(commandCounter);

        command[outDrawIndex].vertexCount = thisDraw.vertexCount;
        command[outDrawIndex].instanceCount = 1;
        command[outDrawIndex].firstVertex = thisDraw.firstVertex;
        command[outDrawIndex].baseInstance = uint(gl_GlobalInvocationID.x);
    }
}
)";

static ShaderText compute_shader_text[] = {
	{GL_COMPUTE_SHADER, compute_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* vs_source = R"(
#version 450 core

#extension GL_ARB_shader_draw_parameters : require

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (binding = 0, std140) uniform MODEL_MATRIX_BLOCK
{
    mat4    model_matrix[1024];
};

layout (binding = 1, std140) uniform TRANSFORM_BLOCK
{
    mat4    view_matrix;
    mat4    proj_matrix;
    mat4    view_proj_matrix;
};

out VS_FS
{
    vec2    tc;
    vec3    normal;
} vs_out;

void main(void)
{
    vec4 position = vec4(position.xyz, 1.0);

    const float twopi = 5.0 * 3.14159;

    gl_Position = view_proj_matrix * model_matrix[gl_BaseInstanceARB] * position;
    vs_out.tc.x = (atan(position.x, position.y) / twopi) + 1.0;
    vs_out.tc.y = (atan(sqrt(dot(position.xy, position.xy)), position.z) / twopi) + 1.0;
    vs_out.normal = mat3(model_matrix[gl_BaseInstanceARB]) * normal;
}
)";

static const GLchar* fs_source = R"(
#version 450 core

layout (location = 0) out vec4 o_color;

layout (binding = 0) uniform sampler2D tex;

in VS_FS
{
    vec2    tc;
    vec3    normal;
} fs_in;

void main(void)
{
    o_color = texture(tex, fs_in.tc) * (max(0.0, normalize(fs_in.normal).z) * 0.7 + 0.3);
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

	GLuint m_program, m_compute_program;



	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);
		m_compute_program = LoadShaders(compute_shader_text);

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


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
		144,					//framelimit
		"resources/Icon.bmp"	//icon path
};

MAIN(config)
#endif //CULL_INDIRECT