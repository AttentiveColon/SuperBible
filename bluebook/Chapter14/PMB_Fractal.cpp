#include "Defines.h"
#ifdef PMB_FRACTAL
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include <chrono>
#include <omp.h>

static const GLchar* vs_source = R"(
#version 440 core

out vec2 uv;

void main(void)
{
    vec2 pos = vec2(float(gl_VertexID & 2), float(gl_VertexID * 2 & 2));
    uv = pos * 0.5;
    gl_Position = vec4(pos - vec2(1.0), 0.0, 1.0);
}
)";

static const GLchar* fs_source = R"(
#version 440 core

layout (binding = 0) uniform sampler2D tex;

in vec2 uv;
layout (location = 0) out vec4 color;

void main(void)
{
    color = texture(tex, uv).xxxx;
}
)";

static ShaderText shader_text[] = {
    {GL_VERTEX_SHADER, vs_source, NULL},
    {GL_FRAGMENT_SHADER, fs_source, NULL},
    {GL_NONE, NULL, NULL}
};


struct Application : public Program {
    float m_clear_color[4];
    u64 m_fps;
    f64 m_time;

    GLuint m_program;
    GLuint m_buffer;

    Application()
        :m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
        m_fps(0),
        m_time(0.0)
    {}

    void OnInit(Input& input, Audio& audio, Window& window) {
        m_program = LoadShaders(shader_text);
  
    }
    void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
        m_fps = window.GetFPS();
        m_time = window.GetTime();
    }
    void OnDraw() {
        int isSignaled;
        glViewport(0, 0, 1600, 900);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    }
    void OnGui() {
        ImGui::Begin("User Defined Settings");
        ImGui::Text("FPS: %d", m_fps);
        ImGui::Text("Time: %f", m_time);
        ImGui::ColorEdit4("Clear Color", m_clear_color);
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
#endif //PMB_FRACTAL