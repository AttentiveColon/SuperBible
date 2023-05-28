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

layout (location = 0) uniform float cull_zone = 1.0;

void main(void)
{
    const CandidateDraw thisDraw = draw[gl_GlobalInvocationID.x];
    const mat4 thisModelMatrix = model_matrix[gl_GlobalInvocationID.x];

    vec4 position = view_proj_matrix * thisModelMatrix * vec4(thisDraw.sphereCenter, 1.0);

    if ((abs(position.x) - thisDraw.sphereRadius) < (position.w * cull_zone) &&
        (abs(position.y) - thisDraw.sphereRadius) < (position.w * cull_zone))
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

struct CandidateDraw
{
    glm::vec3 sphereCenter;
    float sphereRadius;
    unsigned int firstVertex;
    unsigned int vertexCount;
    unsigned int : 32;
    unsigned int : 32;
};

struct DrawArraysIndirectCommand
{
    GLuint vertexCount;
    GLuint instanceCount;
    GLuint firstVertex;
    GLuint baseInstance;
};

struct TransformBuffer
{
    glm::mat4     view_matrix;
    glm::mat4     proj_matrix;
    glm::mat4     view_proj_matrix;
};

#define CANDIDATE_COUNT 1024

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program, m_compute_program;

    struct
    {
        GLuint m_parameters;
        GLuint m_drawCandidates;
        GLuint m_drawCommands;
        GLuint m_modelMatrices;
        GLuint m_transforms;
    } buffers;

    ObjMesh m_object;
    GLuint m_tex;

    float m_cull_zone = 1.0f;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_program = LoadShaders(shader_text);
		m_compute_program = LoadShaders(compute_shader_text);
        m_object.Load_OBJ("./resources/cube.obj");
        m_tex = Load_KTX("./resources/face1.ktx");

        glGenBuffers(1, &buffers.m_parameters);
        glBindBuffer(GL_PARAMETER_BUFFER_ARB, buffers.m_parameters);
        glBufferStorage(GL_PARAMETER_BUFFER_ARB, 256, nullptr, 0);

        glGenBuffers(1, &buffers.m_drawCandidates);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.m_drawCandidates);

        CandidateDraw* pDraws = new CandidateDraw[CANDIDATE_COUNT];

        int i;

        for (i = 0; i < CANDIDATE_COUNT; i++)
        {
            
            pDraws[i].sphereCenter = glm::vec3(0.0f);
            pDraws[i].sphereRadius = 4.0f;
            pDraws[i].firstVertex = 0;
            pDraws[i].vertexCount = m_object.m_count;
        }

        glBufferStorage(GL_SHADER_STORAGE_BUFFER, CANDIDATE_COUNT * sizeof(CandidateDraw), pDraws, 0);

        delete[] pDraws;

        glGenBuffers(1, &buffers.m_drawCommands);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.m_drawCommands);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, CANDIDATE_COUNT * sizeof(DrawArraysIndirectCommand), nullptr, GL_MAP_READ_BIT);

        glGenBuffers(1, &buffers.m_modelMatrices);
        glBindBuffer(GL_UNIFORM_BUFFER, buffers.m_modelMatrices);
        glBufferStorage(GL_UNIFORM_BUFFER, 1024 * sizeof(glm::mat4), nullptr, GL_MAP_WRITE_BIT);

        glGenBuffers(1, &buffers.m_transforms);
        glBindBuffer(GL_UNIFORM_BUFFER, buffers.m_transforms);
        glBufferStorage(GL_UNIFORM_BUFFER, sizeof(TransformBuffer), nullptr, GL_MAP_WRITE_BIT);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, buffers.m_parameters);
        glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, sizeof(GLuint), GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers.m_drawCandidates);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers.m_drawCommands);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, buffers.m_modelMatrices);
        glm::mat4* pModelMatrix = (glm::mat4*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, 1024 * sizeof(glm::mat4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        for (int i = 0; i < 1024; i++)
        {
            float f = float(i) / 127.0f + (float)m_time * 0.025f;
            float g = float(i) / 127.0f;
            const glm::mat4 model_matrix = glm::translate(70.0f * glm::vec3(sinf(f * 3.0f), cosf(f * 5.0f), cosf(f * 9.0f))) *
                glm::rotate((float)m_time * 1.4f, glm::normalize(glm::vec3(sinf(g * 35.0f), cosf(g * 75.0f), cosf(g * 39.0f))));
            pModelMatrix[i] = model_matrix;
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);

        glBindBufferBase(GL_UNIFORM_BUFFER, 1, buffers.m_transforms);
        TransformBuffer* pTransforms = (TransformBuffer*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(TransformBuffer), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        float t = (float)m_time * 0.1f;
        
        const glm::mat4 view_matrix = glm::lookAt(glm::vec3(150.0f * cosf(t), 0.0f, 150.0f * sinf(t)),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 proj_matrix = glm::perspective(0.5f,
            16.0f/9.0f ,
            1.0f,
            2000.0f);

        pTransforms->view_matrix = view_matrix;
        pTransforms->proj_matrix = proj_matrix;
        pTransforms->view_proj_matrix = proj_matrix * view_matrix;

        glUnmapBuffer(GL_UNIFORM_BUFFER);

        glUseProgram(m_compute_program);
        glUniform1f(0, m_cull_zone);
        glDispatchCompute(CANDIDATE_COUNT / 16, 1, 1);
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glBindVertexArray(m_object.m_vao);
        glBindTextureUnit(0, m_tex);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffers.m_drawCommands);
        glBindBuffer(GL_PARAMETER_BUFFER_ARB, buffers.m_parameters);

        glUseProgram(m_program);
        glMultiDrawArraysIndirectCountARB(GL_TRIANGLES, 0, 0, CANDIDATE_COUNT, 0);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
        ImGui::DragFloat("Cull Range", &m_cull_zone, 0.01f);
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