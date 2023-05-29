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

#define FRACTAL_WIDTH 512
#define FRACTAL_HEIGHT 512
#define BUFFER_SIZE FRACTAL_WIDTH * FRACTAL_HEIGHT


struct Application : public Program {
    float m_clear_color[4];
    u64 m_fps;
    f64 m_time;

    GLuint m_program;
    GLuint m_vao;
    GLuint m_buffer;
    GLuint m_texture;
    unsigned char* mapped_buffer;

    int m_max_threads = 0;

    struct {
        glm::vec2 C;
        glm::vec2 offset;
        float zoom;
    } fractparams;

    Application()
        :m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
        m_fps(0),
        m_time(0.0)
    {}

    void OnInit(Input& input, Audio& audio, Window& window) {
        m_max_threads = omp_get_max_threads();
        omp_set_num_threads(m_max_threads);

        m_program = LoadShaders(shader_text);
        
        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_buffer);

        glBufferStorage(GL_PIXEL_UNPACK_BUFFER,
            BUFFER_SIZE,
            nullptr,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        mapped_buffer = (unsigned char*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,
            0,
            BUFFER_SIZE,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, FRACTAL_WIDTH, FRACTAL_HEIGHT);

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
    }
    void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
        m_fps = window.GetFPS();
        m_time = window.GetTime();
    }
    void OnDraw() {
        glViewport(0, 0, 1600, 900);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float nowTime = (float)m_time * 0.01f;

        fractparams.C = glm::vec2(1.5f - cosf(nowTime * 0.4f) * 0.5f,
            1.5f + cosf(nowTime * 0.5f) * 0.5f) * 0.3f;
        fractparams.offset = glm::vec2(cosf(nowTime * 0.14f),
            cosf(nowTime * 0.25f)) * 0.25f;
        fractparams.zoom = (sinf(nowTime) + 1.3f) * 0.7f;

        update_fractal();

        glUseProgram(m_program);
        glBindTextureUnit(0, m_texture);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_buffer);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FRACTAL_WIDTH, FRACTAL_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    void OnGui() {
        ImGui::Begin("User Defined Settings");
        ImGui::Text("FPS: %d", m_fps);
        ImGui::Text("Time: %f", m_time);
        ImGui::ColorEdit4("Clear Color", m_clear_color);
        ImGui::End();
    }
    void update_fractal() {
        const glm::vec2 C = fractparams.C;
        const float thresh_squared = 256.0f;
        const float zoom = fractparams.zoom;
        const glm::vec2 offset = fractparams.offset;

#pragma omp parallel for schedule (dynamic) num_threads(m_max_threads)
        for (int y = 0; y < FRACTAL_HEIGHT; y++)
        {
            for (int x = 0; x < FRACTAL_WIDTH; x++)
            {
                glm::vec2 Z;
                Z[0] = zoom * (float(x) / float(FRACTAL_WIDTH) - 0.5f) + offset[0];
                Z[1] = zoom * (float(y) / float(FRACTAL_HEIGHT) - 0.5f) + offset[1];
                unsigned char* ptr = mapped_buffer + y * FRACTAL_WIDTH + x;

                int it;
                for (it = 0; it < 256; it++)
                {
                    glm::vec2 Z_squared;

                    Z_squared[0] = Z[0] * Z[0] - Z[1] * Z[1];
                    Z_squared[1] = 2.0f * Z[0] * Z[1];
                    Z = Z_squared + C;

                    if ((Z[0] * Z[0] + Z[1] * Z[1]) > thresh_squared)
                        break;
                }
                *ptr = it;
            }
        }
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