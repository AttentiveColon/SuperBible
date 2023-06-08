#include "Defines.h"
#ifdef HDR_BLOOM
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* default_vertex_shader_source = R"(
#version 450 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;

out VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
    flat int material_index;
} vs_out;

// Position of light
uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);

layout (binding = 0, std140) uniform TRANSFORM_BLOCK
{
    mat4    mat_proj;
    mat4    mat_view;
    mat4    mat_model[32];
} transforms;

void main(void)
{
    mat4    mat_mv = transforms.mat_view * transforms.mat_model[gl_InstanceID];

    // Calculate view-space coordinate
    vec4 P = mat_mv * position;

    // Calculate normal in view-space
    vs_out.N = mat3(mat_mv) * normal;

    // Calculate light vector
    vs_out.L = light_pos - P.xyz;

    // Calculate view vector
    vs_out.V = -P.xyz;

    // Pass material index
    vs_out.material_index = gl_InstanceID;

    // Calculate the clip-space position of each vertex
    gl_Position = transforms.mat_proj * P;
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

layout (location = 0) out vec4 color0;
layout (location = 1) out vec4 color1;

in VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
    flat int material_index;
} fs_in;

// Material properties
layout (location = 5)
uniform float bloom_thresh_min = 0.8;
layout (location = 6)
uniform float bloom_thresh_max = 1.2;

struct material_t
{
    vec3    diffuse_color;
    vec3    specular_color;
    float   specular_power;
    vec3    ambient_color;
};

layout (binding = 1, std140) uniform MATERIAL_BLOCK
{
    material_t  material[32];
} materials;

void main(void)
{
    // Normalize the incoming N, L and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);

    // Calculate R locally
    vec3 R = reflect(-L, N);

    material_t m = materials.material[fs_in.material_index];

    // Compute the diffuse and specular components for each fragment
    vec3 diffuse = max(dot(N, L), 0.0) * m.diffuse_color;
    vec3 specular = pow(max(dot(R, V), 0.0), m.specular_power) * m.specular_color;
    vec3 ambient = m.ambient_color;

    // Add ambient, diffuse and specular to find final color
    vec3 color = ambient + diffuse + specular;

    // Write final color to the framebuffer
    color0 = vec4(color, 1.0);

    // Calculate luminance
    float Y = dot(color, vec3(0.299, 0.587, 0.144));

    // Threshold color based on its luminance and write it to
    // the second output
    color = color * 4.0 * smoothstep(bloom_thresh_min, bloom_thresh_max, Y);
    color1 = vec4(color, 1.0);
}
)";

static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* resolve_vertex_shader_source = R"(
#version 450 core

void main(void)
{
    const vec4 vertices[] = vec4[](vec4(-1.0, -1.0, 0.5, 1.0),
                                   vec4( 1.0, -1.0, 0.5, 1.0),
                                   vec4(-1.0,  1.0, 0.5, 1.0),
                                   vec4( 1.0,  1.0, 0.5, 1.0));

    gl_Position = vertices[gl_VertexID];
}
)";

static const GLchar* resolve_fragment_shader_source = R"(
#version 450 core

layout (binding = 0) uniform sampler2D hdr_image;
layout (binding = 1) uniform sampler2D bloom_image;

layout (location = 5)
uniform float exposure = 0.9;
layout (location = 6)
uniform float bloom_factor = 1.0;
layout (location = 7)
uniform float scene_factor = 1.0;

out vec4 color;

void main(void)
{
    vec4 c = vec4(0.0);

    c += texelFetch(hdr_image, ivec2(gl_FragCoord.xy), 0) * scene_factor;
    c += texelFetch(bloom_image, ivec2(gl_FragCoord.xy), 0) * bloom_factor;

    c.rgb = vec3(1.0) - exp(-c.rgb * exposure);
    color = c;
}
)";

static ShaderText resolve_shader_text[] = {
    {GL_VERTEX_SHADER, resolve_vertex_shader_source, NULL},
    {GL_FRAGMENT_SHADER, resolve_fragment_shader_source, NULL},
    {GL_NONE, NULL, NULL}
};

static const GLchar* filter_vertex_shader_source = R"(
#version 450 core

void main(void)
{
    const vec4 vertices[] = vec4[](vec4(-1.0, -1.0, 0.5, 1.0),
                                   vec4( 1.0, -1.0, 0.5, 1.0),
                                   vec4(-1.0,  1.0, 0.5, 1.0),
                                   vec4( 1.0,  1.0, 0.5, 1.0));

    gl_Position = vertices[gl_VertexID];
}
)";

static const GLchar* filter_fragment_shader_source = R"(
#version 450 core

layout (binding = 0) uniform sampler2D hdr_image;

out vec4 color;

const float weights[] = float[](0.0024499299678342,
                                0.0043538453346397,
                                0.0073599963704157,
                                0.0118349786570722,
                                0.0181026699707781,
                                0.0263392293891488,
                                0.0364543006660986,
                                0.0479932050577658,
                                0.0601029809166942,
                                0.0715974486241365,
                                0.0811305381519717,
                                0.0874493212267511,
                                0.0896631113333857,
                                0.0874493212267511,
                                0.0811305381519717,
                                0.0715974486241365,
                                0.0601029809166942,
                                0.0479932050577658,
                                0.0364543006660986,
                                0.0263392293891488,
                                0.0181026699707781,
                                0.0118349786570722,
                                0.0073599963704157,
                                0.0043538453346397,
                                0.0024499299678342);

void main(void)
{
    vec4 c = vec4(0.0);
    ivec2 P = ivec2(gl_FragCoord.yx) - ivec2(0, weights.length() >> 1);
    int i;

    for (i = 0; i < weights.length(); i++)
    {
        c += texelFetch(hdr_image, P + ivec2(0, i), 0) * weights[i];
    }

    color = c;
}
)";

static ShaderText filter_shader_text[] = {
    {GL_VERTEX_SHADER, filter_vertex_shader_source, NULL},
    {GL_FRAGMENT_SHADER, filter_fragment_shader_source, NULL},
    {GL_NONE, NULL, NULL}
};

enum {
    MAX_SCENE_WIDTH = 2048,
    MAX_SCENE_HEIGHT = 2048,
    SPHERE_COUNT = 32,
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

    ObjMesh m_cube;

    GLuint m_vao;
	GLuint m_program, m_program_filter, m_program_resolve;

    GLuint m_render_fbo;
    GLuint m_filter_fbo[2];

    GLuint m_tex_scene, m_tex_bright, m_tex_depth;
    GLuint m_tex_filter[2];
    GLuint m_tex_lut;

    GLuint m_ubo_transform, m_ubo_material;

    float       bloom_thresh_min = 0.8f;
    float       bloom_thresh_max = 1.2f;

    float m_exposure = 1.0f;
    float m_bloom_factor = 1.0f;
    bool m_show_prefilter = false;
    bool m_show_bloom = true;
    bool m_show_scene = true;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
        static const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

		m_program = LoadShaders(default_shader_text);
        m_program_resolve = LoadShaders(resolve_shader_text);
        m_program_filter = LoadShaders(filter_shader_text);

        static const GLfloat exposureLUT[20] = { 11.0f, 6.0f, 3.2f, 2.8f, 2.2f, 1.90f, 1.80f, 1.80f, 1.70f, 1.70f,  1.60f, 1.60f, 1.50f, 1.50f, 1.40f, 1.40f, 1.30f, 1.20f, 1.10f, 1.00f };

        glGenFramebuffers(1, &m_render_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_render_fbo);

        glGenTextures(1, &m_tex_scene);
        glBindTexture(GL_TEXTURE_2D, m_tex_scene);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, MAX_SCENE_WIDTH, MAX_SCENE_HEIGHT);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_tex_scene, 0);

        glGenTextures(1, &m_tex_bright);
        glBindTexture(GL_TEXTURE_2D, m_tex_bright);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, MAX_SCENE_WIDTH, MAX_SCENE_HEIGHT);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_tex_bright, 0);

        glGenTextures(1, &m_tex_depth);
        glBindTexture(GL_TEXTURE_2D, m_tex_depth);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, MAX_SCENE_WIDTH, MAX_SCENE_HEIGHT);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_tex_depth, 0);

        glDrawBuffers(2, buffers);

        glGenFramebuffers(2, &m_filter_fbo[0]);
        glGenTextures(2, &m_tex_filter[0]);
        for (int i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_filter_fbo[i]);
            glBindTexture(GL_TEXTURE_2D, m_tex_filter[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, i ? MAX_SCENE_WIDTH : MAX_SCENE_HEIGHT, i ? MAX_SCENE_HEIGHT : MAX_SCENE_WIDTH);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_tex_filter[i], 0);
            glDrawBuffers(1, buffers);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glGenTextures(1, &m_tex_lut);
        glBindTexture(GL_TEXTURE_1D, m_tex_lut);
        glTexStorage1D(GL_TEXTURE_1D, 1, GL_R32F, 20);
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 20, GL_RED, GL_FLOAT, exposureLUT);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

        m_cube.Load_OBJ("./resources/cube.obj");

        glGenBuffers(1, &m_ubo_transform);
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_transform);
        glBufferData(GL_UNIFORM_BUFFER, (2 + SPHERE_COUNT) * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);

        struct material
        {
            glm::vec3     diffuse_color;
            unsigned int : 32;           // pad
            glm::vec3     specular_color;
            float           specular_power;
            glm::vec3     ambient_color;
            unsigned int : 32;           // pad
        };

        glGenBuffers(1, &m_ubo_material);
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_material);
        glBufferData(GL_UNIFORM_BUFFER, SPHERE_COUNT * sizeof(material), NULL, GL_STATIC_DRAW);

        material* m = (material*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, SPHERE_COUNT * sizeof(material), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        float ambient = 0.002f;
        for (int i = 0; i < SPHERE_COUNT; i++)
        {
            float fi = 3.14159267f * (float)i / 8.0f;
            m[i].diffuse_color = glm::vec3(sinf(fi) * 0.5f + 0.5f, sinf(fi + 1.345f) * 0.5f + 0.5f, sinf(fi + 2.567f) * 0.5f + 0.5f);
            m[i].specular_color = glm::vec3(2.8f, 2.8f, 2.9f);
            m[i].specular_power = 30.0f;
            m[i].ambient_color = glm::vec3(ambient * 0.025f);
            ambient *= 1.5f;
        }
        glUnmapBuffer(GL_UNIFORM_BUFFER);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
        static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        static const GLfloat one = 1.0f;

		m_fps = window.GetFPS();
		m_time = window.GetTime() * 0.01;

        glViewport(0, 0, window.GetWindowDimensions().width, window.GetWindowDimensions().height);

        glBindFramebuffer(GL_FRAMEBUFFER, m_render_fbo);
        glClearBufferfv(GL_COLOR, 0, black);
        glClearBufferfv(GL_COLOR, 1, black);
        glClearBufferfv(GL_DEPTH, 0, &one);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glUseProgram(m_program);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo_transform);
        struct transforms_t
        {
            glm::mat4 mat_proj;
            glm::mat4 mat_view;
            glm::mat4 mat_model[SPHERE_COUNT];
        } *transforms = (transforms_t*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(transforms_t), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        transforms->mat_proj = glm::perspective(0.9f, (float)window.GetWindowDimensions().width / (float)window.GetWindowDimensions().height, 1.0f, 1000.0f);
        transforms->mat_view = glm::translate(glm::vec3(0.0f, 0.0f, -20.0f));
        for (int i = 0; i < SPHERE_COUNT; i++)
        {
            float fi = 3.141592f * (float)i / 16.0f;
            // float r = cosf(fi * 0.25f) * 0.4f + 1.0f;
            float r = (i & 2) ? 0.6f : 1.5f;
            transforms->mat_model[i] = glm::rotate(sinf((float)m_time + fi * 4.0f), glm::vec3(0.0, 1.0, 0.0)) * glm::translate(glm::vec3(cosf((float)m_time + fi) * 5.0f * r, sinf((float)m_time + fi * 4.0f) * 4.0f, sinf((float)m_time + fi) * 5.0f * r)) *
                glm::rotate(sinf((float)m_time + fi * 2.13f) * 5.0f, glm::vec3(1.0, 0.0, 0.0)) * glm::rotate(cosf((float)m_time + fi * 1.37f), glm::vec3(0.0, 1.0, 0.0)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
        }
        
        glUnmapBuffer(GL_UNIFORM_BUFFER);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_ubo_material);

        glUniform1f(5, bloom_thresh_min);
        glUniform1f(6, bloom_thresh_max);

        m_cube.OnDraw(SPHERE_COUNT);

        glDisable(GL_DEPTH_TEST);

        glUseProgram(m_program_filter);

        glBindVertexArray(m_vao);

        glBindFramebuffer(GL_FRAMEBUFFER, m_filter_fbo[0]);
        glBindTexture(GL_TEXTURE_2D, m_tex_bright);
        glViewport(0, 0, window.GetWindowDimensions().height, window.GetWindowDimensions().width);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindFramebuffer(GL_FRAMEBUFFER, m_filter_fbo[1]);
        glBindTexture(GL_TEXTURE_2D, m_tex_filter[0]);
        glViewport(0, 0, window.GetWindowDimensions().width, window.GetWindowDimensions().height);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        

        glUseProgram(m_program_resolve);

        glUniform1f(5, m_exposure);
        if (m_show_prefilter)
        {
            glUniform1f(6, 0.0f);
            glUniform1f(7, 1.0f);
        }
        else
        {
            glUniform1f(6, m_show_bloom ? m_bloom_factor : 0.0f);
            glUniform1f(7, m_show_scene ? 1.0f : 0.0f);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_tex_filter[1]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_show_prefilter ? m_tex_bright : m_tex_scene);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	void OnDraw() {
		/*static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);*/
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
        ImGui::DragFloat("Bloom threshold min", &bloom_thresh_min, 0.01f, 0.1f, 1.0f);
        ImGui::DragFloat("Bloom threshold max", &bloom_thresh_max, 0.01f, 1.0f, 2.0f);
        ImGui::DragFloat("Exposure", &m_exposure, 0.01f, 0.1f, 5.0f);
        ImGui::DragFloat("Bloom factor", &m_bloom_factor, 0.01f, 0.1f, 5.0f);
        ImGui::Checkbox("Show Prefilter", &m_show_prefilter);
        ImGui::Checkbox("Show Bloom", &m_show_bloom);
        ImGui::Checkbox("Show Scene", &m_show_scene);
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
#endif //HDR_BLOOM
