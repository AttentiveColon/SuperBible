#include "Defines.h"
#ifdef RAY_TRACING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* prepare_vertex_shader_source = R"(
#version 450

out VS_OUT
{
    vec3    ray_origin;
    vec3    ray_direction;
} vs_out;

layout (location = 0) uniform mat4 ray_lookat;
layout (location = 1) uniform vec3 ray_origin;
layout (location = 2) uniform float aspect = 0.75;

uniform vec3 direction_scale = vec3(1.9, 1.9, 1.0);
uniform vec3 direction_bias = vec3(0.0, 0.0, 0.0);

void main(void)
{
    vec4 vertices[4] = vec4[4](vec4(-1.0, -1.0, 1.0, 1.0),
                               vec4( 1.0, -1.0, 1.0, 1.0),
                               vec4(-1.0,  1.0, 1.0, 1.0),
                               vec4( 1.0,  1.0, 1.0, 1.0));
    vec4 pos = vertices[gl_VertexID];

    gl_Position = pos;
    vs_out.ray_origin = ray_origin * vec3(1.0, 1.0, -1.0);
    // vs_out.ray_origin = vec3(0.0);
    vs_out.ray_direction = (ray_lookat * vec4(pos.xyz * direction_scale * vec3(1.0, aspect, 2.0) + direction_bias, 0.0)).xyz;
    // vs_out.ray_direction = pos.xyz * vec3(1.0, aspect, 4.0);
}
)";

static const GLchar* prepare_fragment_shader_source = R"(
#version 450 core

in VS_OUT
{
    vec3    ray_origin;
    vec3    ray_direction;
} fs_in;

layout (location = 0) out vec3 color;
layout (location = 1) out vec3 origin;
layout (location = 2) out vec3 reflected;
layout (location = 3) out vec3 refracted;
layout (location = 4) out vec3 reflected_color;
layout (location = 5) out vec3 refracted_color;

void main(void)
{
    color = vec3(0.0);
    origin = fs_in.ray_origin;
    reflected = fs_in.ray_direction;
    refracted = vec3(0.0);
    reflected_color = vec3(1.0);
    refracted_color = vec3(0.0);
}

)";


static ShaderText prepare_shader_text[] = {
    {GL_VERTEX_SHADER, prepare_vertex_shader_source, NULL},
    {GL_FRAGMENT_SHADER, prepare_fragment_shader_source, NULL},
    {GL_NONE, NULL, NULL}
};

static const GLchar* trace_vertex_shader_source = R"(
#version 450

void main(void)
{
    vec4 vertices[4] = vec4[4](vec4(-1.0, -1.0, 0.5, 1.0),
                               vec4( 1.0, -1.0, 0.5, 1.0),
                               vec4(-1.0,  1.0, 0.5, 1.0),
                               vec4( 1.0,  1.0, 0.5, 1.0));
    vec4 pos = vertices[gl_VertexID];

    gl_Position = pos;
}
)";

static const GLchar* trace_fragment_shader_source = R"(
#version 450

layout (location = 0) out vec3 color;
layout (location = 1) out vec3 position;
layout (location = 2) out vec3 reflected;
layout (location = 3) out vec3 refracted;
layout (location = 4) out vec3 reflected_color;
layout (location = 5) out vec3 refracted_color;

layout (binding = 0) uniform sampler2D tex_origin;
layout (binding = 1) uniform sampler2D tex_direction;
layout (binding = 2) uniform sampler2D tex_color;

struct ray
{
    vec3 origin;
    vec3 direction;
};

struct sphere
{
    vec3 center;
    float radius;
    vec4 color;
};

struct light
{
    vec3 position;
};

layout (std140, binding = 1) uniform SPHERES
{
    sphere      S[128];
};

layout (std140, binding = 2) uniform PLANES
{
    vec4        P[128];
};

layout (std140, binding = 3) uniform LIGHTS
{
    light       L[120];
} lights;

uniform int num_spheres = 7;
uniform int num_planes = 6;
uniform int num_lights = 5;

float intersect_ray_sphere(ray R,
                           sphere S,
                           out vec3 hitpos,
                           out vec3 normal)
{
    vec3 v = R.origin - S.center;
    float B = 2.0 * dot(R.direction, v);
    float C = dot(v, v) - S.radius * S.radius;
    float B2 = B * B;

    float f = B2 - 4.0 * C;

    if (f < 0.0)
        return 0.0;

    f = sqrt(f);
    float t0 = -B + f;
    float t1 = -B - f;
    float t = min(max(t0, 0.0), max(t1, 0.0)) * 0.5;

    if (t == 0.0)
        return 0.0;

    hitpos = R.origin + t * R.direction;
    normal = normalize(hitpos - S.center);

    return t;
}

bool intersect_ray_sphere2(ray R, sphere S, out vec3 hitpos, out vec3 normal)
{
    vec3 v = R.origin - S.center;
    float a = 1.0; // dot(R.direction, R.direction);
    float b = 2.0 * dot(R.direction, v);
    float c = dot(v, v) - (S.radius * S.radius);

    float num = b * b - 4.0 * a * c;

    if (num < 0.0)
        return false;

    float d = sqrt(num);
    float e = 1.0 / (2.0 * a);

    float t1 = (-b - d) * e;
    float t2 = (-b + d) * e;
    float t;

    if (t1 <= 0.0)
    {
        t = t2;
    }
    else if (t2 <= 0.0)
    {
        t = t1;
    }
    else
    {
        t = min(t1, t2);
    }

    if (t < 0.0)
        return false;

    hitpos = R.origin + t * R.direction;
    normal = normalize(hitpos - S.center);

    return true;
}

float intersect_ray_plane(ray R, vec4 P, out vec3 hitpos, out vec3 normal)
{
    vec3 O = R.origin;
    vec3 D = R.direction;
    vec3 N = P.xyz;
    float d = P.w;

    float denom = dot(D, N);

    if (denom == 0.0)
        return 0.0;

    float t = -(d + dot(O, N)) / denom;

    if (t < 0.0)
        return 0.0;

    hitpos = O + t * D;
    normal = N;

    return t;
}

bool point_visible_to_light(vec3 point, vec3 L)
{
    return true;

    int i;
    ray R;
    vec3 normal;
    vec3 hitpos;

    R.direction = normalize(L - point);
    R.origin = point + R.direction * 0.001;

    for (i = 0; i < num_spheres; i++)
    {
        if (intersect_ray_sphere(R, S[i], hitpos, normal) != 0.0)
        {
            return false;
        }
    }

    //*
    for (i = 0; i < num_planes; i++)
    {
        if (intersect_ray_plane(R, P[i], hitpos, normal) != 0.0)
        {
            return false;
        }
    }
    //*/

    return true;
}

vec3 light_point(vec3 position, vec3 normal, vec3 V, light l)
{
    vec3 ambient = vec3(0.0);

    if (!point_visible_to_light(position, l.position))
    {
        return ambient;
    }
    else
    {
        // vec3 V = normalize(-position);
        vec3 L = normalize(l.position - position);
        vec3 N = normal;
        vec3 R = reflect(-L, N);

        float rim = clamp(dot(N, V), 0.0, 1.0);
        rim = smoothstep(0.0, 1.0, 1.0 - rim);
        float diff = clamp(dot(N, L), 0.0, 1.0);
        float spec = pow(clamp(dot(R, N), 0.0, 1.0), 260.0);

        vec3 rim_color = vec3(0.0); // , 0.2, 0.2);
        vec3 diff_color = vec3(0.125); // , 0.8, 0.8);
        vec3 spec_color = vec3(0.1);

        return ambient + rim_color * rim + diff_color * diff + spec_color * spec;
    }
}

void main(void)
{
    ray R;

    R.origin = texelFetch(tex_origin, ivec2(gl_FragCoord.xy), 0).xyz;
    R.direction = normalize(texelFetch(tex_direction, ivec2(gl_FragCoord.xy), 0).xyz);
    vec3 input_color = texelFetch(tex_color, ivec2(gl_FragCoord.xy), 0).rgb;

    vec3 hit_position = vec3(0.0);
    vec3 hit_normal = vec3(0.0);

    color = vec3(0.0);
    position = vec3(0.0);
    reflected = vec3(0.0);
    refracted = vec3(0.0);
    reflected_color = vec3(0.0);
    refracted_color = vec3(0.0);

    if (all(lessThan(input_color, vec3(0.05))))
    {
        return;
    }

    R.origin += R.direction * 0.01;

    ray refl;
    ray refr;
    vec3 hitpos;
    vec3 normal;
    float min_t = 1000000.0f;
    int i;
    int sphere_index = 0;
    float t;

    for (i = 0; i < num_spheres; i++)
    {
        t = intersect_ray_sphere(R, S[i], hitpos, normal);
        if (t != 0.0)
        {
            if (t < min_t)
            {
                min_t = t;
                hit_position = hitpos;
                hit_normal = normal;
                sphere_index = i;
            }
        }
    }

    // int foobar[] = { 1, 0, 0, 0, 0, 0, 0 }; // 1, 1, 1, 1, 1, 1 };
    int foobar[] = { 1, 1, 1, 1, 1, 1, 1 };

    for (i = 0; i < 6; i++)
    {
        t = intersect_ray_plane(R, P[i], hitpos, normal);
        if (foobar[i] != 0 && t != 0.0)
        {
            if (t < min_t)
            {
                min_t = t;
                hit_position = hitpos;
                hit_normal = normal;
                sphere_index = i * 25;
            }
        }
    }

    if (min_t < 100000.0f)
    {
        vec3 my_color = vec3(0.0);

        for (i = 0; i < num_lights; i++)
        {
            my_color += light_point(hit_position, hit_normal, -R.direction, lights.L[i]);
        }

        my_color *= S[sphere_index].color.rgb;
        color = input_color * my_color;
        vec3 v = normalize(hit_position - R.origin);
        position = hit_position;
        reflected = reflect(v, hit_normal);
        reflected_color = /* input_color * */ S[sphere_index].color.rgb * 0.5;
        refracted = refract(v, hit_normal, 1.73);
        refracted_color = input_color * S[sphere_index].color.rgb * 0.5;
    }

    // color = (R.origin.zzz - 40.0) * 0.05;
    // color = R.direction;
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
    vec4 vertices[4] = vec4[4](vec4(-1.0, -1.0, 0.5, 1.0),
                       vec4( 1.0, -1.0, 0.5, 1.0),
                       vec4(-1.0,  1.0, 0.5, 1.0),
                       vec4( 1.0,  1.0, 0.5, 1.0));

    gl_Position = vertices[gl_VertexID];
}

)";

static const GLchar* blit_fragment_shader_source = R"(
#version 420 core

layout (binding = 0) uniform sampler2D tex_composite;

layout (location = 0) out vec4 color;

void main(void)
{
    color = texelFetch(tex_composite, ivec2(gl_FragCoord.xy), 0);
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

#define MAX_RECURSION_DEPTH 20
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

    int m_max_depth = 4;

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
        block->proj_matrix = glm::perspective(0.5f, 16.0f / 9.0f, 0.1f, 1000.0f);

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
        //glUniform1f(2, 16.0f / 9.0f);
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