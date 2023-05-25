#include "Defines.h"
#ifdef RAY_TRACING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* prepare_vertex_shader_source = R"(
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

static const GLchar* prepare_fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main()
{
    color = vec4(1.0);
}
)";


static ShaderText prepare_shader_text[] = {
    {GL_VERTEX_SHADER, prepare_vertex_shader_source, NULL},
    {GL_FRAGMENT_SHADER, prepare_fragment_shader_source, NULL},
    {GL_NONE, NULL, NULL}
};

static const GLchar* trace_vertex_shader_source = R"(
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

static const GLchar* trace_fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main()
{
    color = vec4(1.0);
}
)";


static ShaderText trace_shader_text[] = {
    {GL_VERTEX_SHADER, trace_vertex_shader_source, NULL},
    {GL_FRAGMENT_SHADER, trace_fragment_shader_source, NULL},
    {GL_NONE, NULL, NULL}
};

static const GLchar* blit_vertex_shader_source = R"(
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

static const GLchar* blit_fragment_shader_source = R"(
#version 450 core

out vec4 color;

void main()
{
    color = vec4(1.0);
}
)";


static ShaderText blit_shader_text[] = {
    {GL_VERTEX_SHADER, blit_vertex_shader_source, NULL},
    {GL_FRAGMENT_SHADER, blit_fragment_shader_source, NULL},
    {GL_NONE, NULL, NULL}
};

struct uniforms_block
{
    glm::mat4     mv_matrix;
    glm::mat4     view_matrix;
    glm::mat4     proj_matrix;
};

struct sphere
{
    glm::vec3 center;
    float radius;
    glm::vec4 color;
};

struct plane
{
    glm::vec3 normal;
    float d;
};

struct light
{
    glm::vec3 position;
    float pad;
};

#define MAX_RECURSION_DEPTH 5
#define MAX_FB_WIDTH 2048
#define MAX_FB_HEIGHT 1024

struct Application : public Program {
    float m_clear_color[4];
    u64 m_fps;
    f64 m_time;

    GLuint m_prepare_program, m_trace_program, m_blit_program;

    GLuint m_vao;
    GLuint m_uniforms_buffer, m_sphere_buffer, m_plane_buffer, m_light_buffer;

    GLuint m_ray_fbo[MAX_RECURSION_DEPTH];
    GLuint m_tex_composite;
    GLuint m_tex_position[MAX_RECURSION_DEPTH], m_tex_reflected[MAX_RECURSION_DEPTH], m_tex_refracted[MAX_RECURSION_DEPTH];
    GLuint m_tex_reflection_intensity[MAX_RECURSION_DEPTH], m_tex_refraction_intensity[MAX_RECURSION_DEPTH];

    int m_max_depth = 1;

    Application()
        :m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
        m_fps(0),
        m_time(0.0)
    {}

    void OnInit(Input& input, Audio& audio, Window& window) {
        m_prepare_program = LoadShaders(prepare_shader_text);
        m_trace_program = LoadShaders(trace_shader_text);
        m_blit_program = LoadShaders(blit_shader_text);
        
        glGenBuffers(1, &m_uniforms_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_uniforms_buffer);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(uniforms_block), NULL, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &m_sphere_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_sphere_buffer);
        glBufferData(GL_UNIFORM_BUFFER, 128 * sizeof(sphere), NULL, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &m_plane_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_plane_buffer);
        glBufferData(GL_UNIFORM_BUFFER, 128 * sizeof(plane), NULL, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &m_light_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_light_buffer);
        glBufferData(GL_UNIFORM_BUFFER, 128 * sizeof(sphere), NULL, GL_DYNAMIC_DRAW);

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glGenFramebuffers(MAX_RECURSION_DEPTH, m_ray_fbo);
        glGenTextures(1, &m_tex_composite);
        glGenTextures(MAX_RECURSION_DEPTH, m_tex_position);
        glGenTextures(MAX_RECURSION_DEPTH, m_tex_reflected);
        glGenTextures(MAX_RECURSION_DEPTH, m_tex_refracted);
        glGenTextures(MAX_RECURSION_DEPTH, m_tex_reflection_intensity);
        glGenTextures(MAX_RECURSION_DEPTH, m_tex_refraction_intensity);

        glBindTexture(GL_TEXTURE_2D, m_tex_composite);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, MAX_FB_WIDTH, MAX_FB_HEIGHT);

        for (int i = 0; i < MAX_RECURSION_DEPTH; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_ray_fbo[i]);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_tex_composite, 0);

            glBindTexture(GL_TEXTURE_2D, m_tex_position[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, MAX_FB_WIDTH, MAX_FB_HEIGHT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_tex_position[i], 0);

            glBindTexture(GL_TEXTURE_2D, m_tex_reflected[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, MAX_FB_WIDTH, MAX_FB_HEIGHT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, m_tex_reflected[i], 0);

            glBindTexture(GL_TEXTURE_2D, m_tex_refracted[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, MAX_FB_WIDTH, MAX_FB_HEIGHT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, m_tex_refracted[i], 0);

            glBindTexture(GL_TEXTURE_2D, m_tex_reflection_intensity[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, MAX_FB_WIDTH, MAX_FB_HEIGHT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, m_tex_reflection_intensity[i], 0);

            glBindTexture(GL_TEXTURE_2D, m_tex_refraction_intensity[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, MAX_FB_WIDTH, MAX_FB_HEIGHT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, m_tex_refraction_intensity[i], 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
        m_fps = window.GetFPS();
        m_time = window.GetTime();
    }
    void OnDraw() {
        static const GLfloat zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 0.0f };
        static const GLfloat ones[] = { 1.0f };

        float f = (float)m_time;

        glm::vec3 view_position = glm::vec3(sinf(f * 0.3234f) * 28.0f, cosf(f * 0.4234f) * 28.0f, cosf(f * 0.1234f) * 28.0f);
        glm::vec3 lookat_point = glm::vec3(sinf(f * 0.214f) * 8.0f, cosf(f * 0.153f) * 8.0f, sinf(f * 0.734f) * 8.0f);
        glm::mat4 view_matrix = glm::lookAt(view_position, lookat_point, glm::vec3(0.0f, 1.0f, 0.0f));

        //Setup Camera Buffer
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_uniforms_buffer);
        uniforms_block* block = (uniforms_block*)glMapBufferRange(GL_UNIFORM_BUFFER,
            0,
            sizeof(uniforms_block),
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        glm::mat4 model_matrix = glm::scale(glm::vec3(7.0f));

        block->mv_matrix = view_matrix * model_matrix;
        block->view_matrix = view_matrix;
        block->proj_matrix = glm::perspective(50.0f, 16.0f / 9.0f, 0.1f, 1000.0f);

        glUnmapBuffer(GL_UNIFORM_BUFFER);

        //Setup Sphere Buffer
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_sphere_buffer);
        sphere* s = (sphere*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, 128 * sizeof(sphere), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        for (int i = 0; i < 128; i++)
        {
            // float f = 0.0f;
            float fi = (float)i / 128.0f;
            s[i].center = glm::vec3(sinf(fi * 123.0f + f) * 15.75f, cosf(fi * 456.0f + f) * 15.75f, (sinf(fi * 300.0f + f) * cosf(fi * 200.0f + f)) * 20.0f);
            s[i].radius = fi * 2.3f + 3.5f;
            float r = fi * 61.0f;
            float g = r + 0.25f;
            float b = g + 0.25f;
            r = (r - floorf(r)) * 0.8f + 0.2f;
            g = (g - floorf(g)) * 0.8f + 0.2f;
            b = (b - floorf(b)) * 0.8f + 0.2f;
            s[i].color = glm::vec4(r, g, b, 1.0f);
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);

        //Setup Plane Buffer
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_plane_buffer);
        plane* p = (plane*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, 128 * sizeof(plane), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        //for (i = 0; i < 1; i++)
        {
            p[0].normal = glm::vec3(0.0f, 0.0f, -1.0f);
            p[0].d = 30.0f;

            p[1].normal = glm::vec3(0.0f, 0.0f, 1.0f);
            p[1].d = 30.0f;

            p[2].normal = glm::vec3(-1.0f, 0.0f, 0.0f);
            p[2].d = 30.0f;

            p[3].normal = glm::vec3(1.0f, 0.0f, 0.0f);
            p[3].d = 30.0f;

            p[4].normal = glm::vec3(0.0f, -1.0f, 0.0f);
            p[4].d = 30.0f;

            p[5].normal = glm::vec3(0.0f, 1.0f, 0.0f);
            p[5].d = 30.0f;
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);


        //Setup Light Buffer
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, m_light_buffer);
        light* l = (light*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, 128 * sizeof(light), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        f *= 1.0f;

        for (int i = 0; i < 128; i++)
        {
            float fi = 3.33f - (float)i; //  / 35.0f;
            l[i].position = glm::vec3(sinf(fi * 2.0f - f) * 15.75f,
                cosf(fi * 5.0f - f) * 5.75f,
                (sinf(fi * 3.0f - f) * cosf(fi * 2.5f - f)) * 19.4f);
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);


        //Prepare to render
        glBindVertexArray(m_vao);
        glViewport(0, 0, 1600, 900);

        glUseProgram(m_prepare_program);
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(view_matrix));
        glUniform3fv(1, 1, glm::value_ptr(view_position));
        glUniform1f(2, 1600.0f / 9.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, m_ray_fbo[0]);
        static const GLenum draw_buffers[] =
        {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3,
            GL_COLOR_ATTACHMENT4,
            GL_COLOR_ATTACHMENT5
        };
        glDrawBuffers(6, draw_buffers);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //Ray Trace Scene
        glUseProgram(m_trace_program);
        recurse(0);

        //Display scene
        glUseProgram(m_blit_program);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK);

        glBindTextureUnit(0, m_tex_composite);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    void OnGui() {
        ImGui::Begin("User Defined Settings");
        ImGui::Text("FPS: %d", m_fps);
        ImGui::Text("Time: %f", m_time);
        ImGui::End();
    }
    void recurse(int depth) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ray_fbo[depth + 1]);

        static const GLenum draw_buffers[] =
        {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3,
            GL_COLOR_ATTACHMENT4,
            GL_COLOR_ATTACHMENT5
        };
        glDrawBuffers(6, draw_buffers);

        glEnablei(GL_BLEND, 0);
        glBlendFunci(0, GL_ONE, GL_ONE);

        glBindTextureUnit(0, m_tex_position[depth]);

        glBindTextureUnit(1, m_tex_reflected[depth]);

        glBindTextureUnit(2, m_tex_reflection_intensity[depth]);

        // Render
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (depth != (m_max_depth - 1))
        {
            recurse(depth + 1);
        }

        glDisablei(GL_BLEND, 0);
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