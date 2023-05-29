#include "Defines.h"
#ifdef PMB_STREAMING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include <chrono>
#include <omp.h>


static const GLchar* vs_source = R"(
#version 450 core

#extension GL_ARB_shader_draw_parameters : require

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

struct MATRICES
{
    mat4 modelview;
    mat4 projection;
};

layout (std140) uniform constants
{
    MATRICES matrices[16];
};

out VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
    vec2 tc;
} vs_out;

const vec3 light_pos = vec3(100.0, 100.0, 100.0);

void main(void)
{
    mat4 mv_matrix = matrices[gl_BaseInstanceARB].modelview;
    mat4 proj_matrix = matrices[gl_BaseInstanceARB].projection;

    vec4 P = mv_matrix * position;

    vs_out.N = mat3(mv_matrix) * normal;

    vs_out.L = light_pos - P.xyz;

    vs_out.V = -P.xyz;

    vs_out.tc = uv * vec2(5.0, 1.0);

    gl_Position = proj_matrix * P;
}
)";

static const GLchar* fs_source = R"(
#version 450 core

layout (location = 0) out vec4 color;

layout (binding = 0) uniform sampler2D tex;

in VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
    vec2 tc;
} fs_in;

const vec3 diffuse_albedo = vec3(0.5, 0.5, 0.9);
const vec3 specular_albedo = vec3(0.7);
const float specular_power = 300.0;

void main(void)
{
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);
    vec3 H = normalize(L + V);

    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
    vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;

    float factor = texture(tex, fs_in.tc).x;
    vec4 lit = vec4(mix(diffuse, diffuse.yzx, factor)+ specular, 1.0);
    color = lit;
}
)";

static ShaderText shader_text[] = {
    {GL_VERTEX_SHADER, vs_source, NULL},
    {GL_FRAGMENT_SHADER, fs_source, NULL},
    {GL_NONE, NULL, NULL}
};

struct MATRICES
{
    glm::mat4 model_view;
    glm::mat4 projection;
};

enum
{
    CHUNK_COUNT = 4,
    CHUNK_SIZE = 3 * sizeof(glm::mat4),
    BUFFER_SIZE = (CHUNK_SIZE * CHUNK_COUNT)
};

struct Application : public Program {
    float m_clear_color[4];
    u64 m_fps;
    f64 m_time;

    GLuint m_program;
    GLuint m_buffer;
    GLsync fence[CHUNK_COUNT];
    int sync_index = 0;
    bool stalled;

    MATRICES* vs_uniforms;

    ObjMesh m_object;
    GLuint m_tex;

    int set_mode = 0;
    enum MODE
    {
        NO_SYNC = 0,
        FINISH = 1,
        ONE_SYNC = 2,
        RINGED_SYNC = 3,
        NUM_MODES
    } mode;


    Application()
        :m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
        m_fps(0),
        m_time(0.0)
    {}

    void OnInit(Input& input, Audio& audio, Window& window) {
        m_program = LoadShaders(shader_text);
        m_object.Load_OBJ("./resources/monkey.obj");
        m_tex = Load_KTX("./resources/face1.ktx");
        mode = NO_SYNC;

        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);

        //Create buffer storage and get a persistant pointer to buffer
        glBufferStorage(GL_UNIFORM_BUFFER, BUFFER_SIZE, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        vs_uniforms = (MATRICES*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, BUFFER_SIZE, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_buffer);

        for (int i = 0; i < CHUNK_COUNT; ++i) {
            fence[i] = 0;
        }
    }
    void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
        m_fps = window.GetFPS();
        m_time = window.GetTime();
    }
    void OnDraw() {
        int isSignaled;
        glViewport(0, 0, 1600, 900);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj_matrix = glm::perspective(0.6f, 1600.0f / 900.0f, 0.1f, 1800.0f);
        glm::mat4 mv_matrix = glm::translate(glm::vec3(0.0f, 0.0f, -3.0f)) *
            glm::rotate((float)m_time * .75f, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate((float)m_time * .75f, glm::vec3(0.0f, 0.0f, 1.0f)) *
            glm::rotate((float)m_time * .3f, glm::vec3(1.0f, 0.0f, 0.0f));

        glUseProgram(m_program);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glBindTextureUnit(0, m_tex);

        if (mode == ONE_SYNC)
        {
            if (fence[0] != 0)
            {
                glGetSynciv(fence[0], GL_SYNC_STATUS, sizeof(int), nullptr, &isSignaled);
                stalled = isSignaled == GL_UNSIGNALED;
                glClientWaitSync(fence[0], 0, GL_TIMEOUT_IGNORED);
                glDeleteSync(fence[0]);
            }
        }
        else if (mode == RINGED_SYNC)
        {
            if (fence[sync_index] != 0)
            {
                glGetSynciv(fence[sync_index], GL_SYNC_STATUS, sizeof(int), nullptr, &isSignaled);
                stalled = isSignaled == GL_UNSIGNALED;
                glClientWaitSync(fence[sync_index], 0, GL_TIMEOUT_IGNORED);
                glDeleteSync(fence[sync_index]);
            }
        }

        vs_uniforms[sync_index].model_view = mv_matrix;
        vs_uniforms[sync_index].projection = proj_matrix;

        if (mode == RINGED_SYNC)
        {
            m_object.OnDraw(sync_index);
        }
        else
        {
            m_object.OnDraw();
        }

        if (mode == FINISH)
        {
            glFinish();
            stalled = true;
        }
        else if (mode == ONE_SYNC)
        {
            fence[0] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        }
        else if (mode == RINGED_SYNC)
        {
            fence[sync_index] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        }

        sync_index = (sync_index + 1) % CHUNK_COUNT;
        
    }
    void OnGui() {
        ImGui::Begin("User Defined Settings");
        ImGui::Text("FPS: %d", m_fps);
        ImGui::Text("Time: %f", m_time);
        ImGui::ColorEdit4("Clear Color", m_clear_color);
        ImGui::DragInt("Set Mode", &set_mode, 0.5f, 0, 3);
        if (mode == NO_SYNC)
            ImGui::Text("Current Mode: NO_SYNC");
        if (mode == FINISH)
            ImGui::Text("Current Mode: FINISH");
        if (mode == ONE_SYNC)
            ImGui::Text("Current Mode: ONE_SYNC");
        if (mode == RINGED_SYNC)
            ImGui::Text("Current Mode: RINGED_SYNC");
        if (stalled)
            ImGui::Text("STALLED");

        ImGui::End();

        mode = (MODE)set_mode;
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
#endif //PMB_STREAMING