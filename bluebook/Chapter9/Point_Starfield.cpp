#include "Defines.h"
#ifdef POINT_STARFIELD
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* default_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec4 position;
layout (location = 1)
in vec4 color;

layout (location = 5)
uniform float u_time;
layout (location = 6)
uniform mat4 u_proj;

flat out vec4 star_color;

void main(void)
{
    vec4 newVertex = position;
    
    newVertex.z += u_time;
    newVertex.z = fract(newVertex.z);

    float size = (20.0 * newVertex.z * newVertex.z);

    star_color = smoothstep(1.0, 7.0, size) * color;
    
    newVertex.z = (999.9 * newVertex.z) - 1000.0;
    gl_Position = u_proj * newVertex;
    gl_PointSize = size;
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

layout (location = 0)
out vec4 color;

layout (binding = 0)
uniform sampler2D u_texture;

flat in vec4 star_color;

void main(void)
{
    color = star_color * texture(u_texture, gl_PointCoord);    
}
)";

static ShaderText default_shader_text[] = {
    {GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
    {GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
    {GL_NONE, NULL, NULL}
};

#define NUM_STARS 2000

struct Application : public Program {
    float m_clear_color[4];
    u64 m_fps;
    f64 m_time;

    GLuint m_vao;
    GLuint m_buffer;
    GLuint m_program;

    GLuint m_tex;
    glm::mat4 m_proj;

    Random u_gen;
    int m_width;
    int m_height;

    Application()
        :m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
        m_fps(0),
        m_time(0.0)
    {}

    void OnInit(Input& input, Audio& audio, Window& window) {
        u_gen.Init();

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        m_tex = Load_KTX("./resources/star.ktx");

        m_program = LoadShaders(default_shader_text);
        m_width = window.GetWindowDimensions().width;
        m_height = window.GetWindowDimensions().height;
        m_proj = glm::perspective(0.9f, (float)m_width / (float)m_height, 0.01f, 1000.0f);

        struct star_t {
            glm::vec3 position;
            glm::vec3 color;
        };

        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
        glBufferData(GL_ARRAY_BUFFER, NUM_STARS * sizeof(star_t), NULL, GL_STATIC_DRAW);

        star_t* star = (star_t*)glMapBufferRange(GL_ARRAY_BUFFER, 0, NUM_STARS * sizeof(star_t), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        for (int i = 0; i < 1000; ++i) {
            star[i].position.x = (u_gen.Float() * 2.0 - 1.0f) * 100.0f;
            star[i].position.y = (u_gen.Float() * 2.0f - 1.0f) * 100.0f;
            star[i].position.z = u_gen.Float();
            star[i].color.r = 0.8f + u_gen.Float() * 0.2f;
            star[i].color.g = 0.8f + u_gen.Float() * 0.2f;
            star[i].color.b = 0.8f + u_gen.Float() * 0.2f;
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(star_t), NULL);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(star_t), (void*)sizeof(glm::vec3));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }
    void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
        m_fps = window.GetFPS();
        m_time = window.GetTime();
    }
    void OnDraw() {
        float t = (float)m_time * 0.1f;
        t -= floor(t);

        static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        static const GLfloat one = 1.0f;
        glViewport(0, 0, m_width, m_height);

        glClearBufferfv(GL_COLOR, 0, black);
        glClearBufferfv(GL_DEPTH, 0, &one);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glUseProgram(m_program);
        glUniform1f(5, t);
        glUniformMatrix4fv(6, 1, GL_FALSE, glm::value_ptr(m_proj));

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glBindVertexArray(m_vao);
        glDrawArrays(GL_POINTS, 0, NUM_STARS);
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
#endif //POINT_STARFIELD
