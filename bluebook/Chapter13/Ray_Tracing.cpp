#include "Defines.h"
#ifdef RAY_TRACING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* vertex_shader_source = R"(
#version 450 core

void main(void)
{
    const vec4 vertices[] = vec4[]( vec4(-1.0, -1.0, 0.5, 1.0),
                                    vec4( 1.0, -1.0, 0.5, 1.0),
                                    vec4(-1.0,  1.0, 0.5, 1.0),
                                    vec4( 1.0,  1.0, 0.5, 1.0) );

    gl_Position = vertices[gl_VertexID];
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main()
{
    color = vec4(1.0);
}
)";


static ShaderText shader_text[] = {
    {GL_VERTEX_SHADER, vertex_shader_source, NULL},
    {GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
    {GL_NONE, NULL, NULL}
};

struct Application : public Program {
    float m_clear_color[4];
    u64 m_fps;
    f64 m_time;

    GLuint m_program;

    Application()
        :m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
        m_fps(0),
        m_time(0.0)
    {}

    void OnInit(Input& input, Audio& audio, Window& window) {
        m_program = LoadShaders(shader_text);
        glEnable(GL_DEPTH_TEST);

        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    }
    void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
        m_fps = window.GetFPS();
        m_time = window.GetTime();
    }
    void OnDraw() {
        glViewport(0, 0, 1600, 900);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glUseProgram(m_program);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    void OnGui() {
        ImGui::Begin("User Defined Settings");
        ImGui::Text("FPS: %d", m_fps);
        ImGui::Text("Time: %f", m_time);
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
#endif //RAY_TRACING