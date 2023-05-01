#include "Defines.h"
#ifdef ROTATING_POINTS
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* default_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
uniform float angle;

flat out int shape;
flat out float sin_theta;
flat out float cos_theta;

void main(void)
{
    const vec4 array[4] = vec4[4](
        vec4(-0.5, 0.5, 0.5, 1.0),
        vec4(0.5, 0.5, 0.5, 1.0),
        vec4(-0.5, -0.5, 0.5, 1.0),
        vec4(0.5, -0.5, 0.5, 1.0)
    );

    gl_Position = array[gl_VertexID];
    gl_PointSize = 200.0;
    shape = gl_VertexID;

    sin_theta = sin(angle);
    cos_theta = cos(angle);
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

flat in int shape;
flat in float sin_theta;
flat in float cos_theta;

out vec4 color;

void main(void)
{
    color = vec4(1.0); 
    vec2 p = gl_PointCoord * 2.0 - vec2(1.0);

    mat2 rotation_matrix = mat2(cos_theta, sin_theta,
                                -sin_theta, cos_theta);

    p = rotation_matrix * p;

    if (shape == 0)
    {
        if (dot(p, p) > 1.0)
            discard;
    } 
    else if (shape == 1)
    {
        if (abs(0.8 - dot(p, p)) > 0.2)
            discard;
    }
    else if (shape == 2)
    {
        if (dot(p,p) > sin(atan(p.y, p.x) * 5.0))
            discard; 
    }
    else if (shape == 3)
    {
        if (abs(p.x) < abs(p.y))
            discard;
    }
}
)";

static ShaderText default_shader_text[] = {
    {GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
    {GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
    {GL_NONE, NULL, NULL}
};


struct Application : public Program {
    float m_clear_color[4];
    u64 m_fps;
    f64 m_time;

    GLuint m_vao;
    GLuint m_program;

    int m_width;
    int m_height;

    Application()
        :m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
        m_fps(0),
        m_time(0.0)
    {}

    void OnInit(Input& input, Audio& audio, Window& window) {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        m_program = LoadShaders(default_shader_text);
        m_width = window.GetWindowDimensions().width;
        m_height = window.GetWindowDimensions().height;
    }
    void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
        m_fps = window.GetFPS();
        m_time = window.GetTime();
    }
    void OnDraw() {
        static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        static const GLfloat one = 1.0f;
        glViewport(0, 0, m_width, m_height);

        glClearBufferfv(GL_COLOR, 0, black);
        glClearBufferfv(GL_DEPTH, 0, &one);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glUseProgram(m_program);
        glUniform1f(0, (float)m_time);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_POINTS, 0, 4);
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
#endif //ROTATING_POINTS
