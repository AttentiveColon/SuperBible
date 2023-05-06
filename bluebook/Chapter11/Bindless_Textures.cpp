#include "Defines.h"
#ifdef BINDLESS_TEXTURES
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* default_vertex_shader_source = R"(
#version 450 core

#extension GL_ARB_shader_draw_parameters : require

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tc;

layout (std140, binding = 0) uniform MATRIX_BLOCK
{
#if 0
    mat4 view_matrix;
    mat4 proj_matrix;
    mat4 model_matrix[384];
#else
    mat4 model_matrix[386];
#endif
};

// 

out VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
    vec2 tc;
    flat uint instance_index;
} vs_out;

const vec3 light_pos = vec3(100.0, 100.0, 100.0);

void main(void)
{
#if 1
    mat4 view_matrix = model_matrix[0];
    mat4 proj_matrix = model_matrix[1];
#endif

    mat4 mv_matrix = view_matrix * model_matrix[gl_InstanceID + 2];
    // mat4 proj_matrix = proj_matrix;


    vec4 P = mv_matrix * position;


    vs_out.N = mat3(mv_matrix) * normal;

    // Calculate light vector
    vs_out.L = light_pos - P.xyz;

    // Calculate view vector
    vs_out.V = -P.xyz;

    // Pass texture coordinate through
    vs_out.tc = tc * vec2(5.0, 1.0);

    // Pass instance ID through
    vs_out.instance_index = gl_InstanceID;

    // Calculate the clip-space position of each vertex
    gl_Position = proj_matrix * P;
}
)";

static const GLchar* default_fragment_shader_source = R"(
#version 450 core

#extension GL_ARB_bindless_texture : require

layout (location = 0) out vec4 color;

in VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
    vec2 tc;
    flat uint instance_index;
} fs_in;

const vec3 ambient = vec3(0.1, 0.1, 0.1);
const vec3 diffuse_albedo = vec3(0.9, 0.9, 0.9);
const vec3 specular_albedo = vec3(0.7);
const float specular_power = 300.0;

layout (binding = 1, std140) uniform TEXTURE_BLOCK
{
    sampler2D      tex[384];
};

void main(void)
{
    // Normalize the incoming N, L and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);
    vec3 H = normalize(L + V);

    // Compute the diffuse and specular components for each fragment
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
    // This is where we reference the bindless texture
    diffuse *= texture(tex[fs_in.instance_index], fs_in.tc * 2.0).rgb;
    vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;

    // Write final color to the framebuffer
    color = vec4(ambient + diffuse + specular, 1.0);
}
)";

static ShaderText default_shader_text[] = {
	{GL_VERTEX_SHADER, default_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, default_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

enum
{
    NUM_TEXTURES = 384,
    TEXTURE_LEVELS = 5,
    TEXTURE_SIZE = (1 << (TEXTURE_LEVELS - 1))
};

struct MATRICES
{
    glm::mat4     view;
    glm::mat4     projection;
    glm::mat4     model[NUM_TEXTURES];
};

unsigned int get_random_uint(Random& random) {
    float r = random.Float();
    return (unsigned int)(UINT_MAX * r);
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
    Random m_random;

	GLuint m_vao;
    GLuint m_program;

    ObjMesh m_cube;

    GLuint m_transform_buffer, m_texture_handle_buffer;

    SB::Camera m_camera;
    bool m_input_mode = false;

    struct
    {
        GLuint      name;
        GLuint64    handle;
    } textures[1024];

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
        m_program = LoadShaders(default_shader_text);
        m_cube.Load_OBJ("./resources/cube.obj");
        m_camera = SB::Camera("Camera", glm::vec3(0.0f, 2.0f, -10.0f), glm::vec3(0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
        m_random.Init();

        unsigned char tex_data[32 * 32 * 4];
        unsigned int mutated_data[32 * 32];
        memset(tex_data, 0, sizeof(tex_data));

        for (int i = 0; i < 32; ++i) {
            for (int j = 0; j < 32; ++j) {
                tex_data[i * 4 * 32 + j * 4] = (i ^ j) << 3;
                tex_data[i * 4 * 32 + j * 4 + 1] = (i ^ j) << 3;
                tex_data[i * 4 * 32 + j * 4 + 2] = (i ^ j) << 3;
            }
        }

        glGenBuffers(1, &m_transform_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_transform_buffer);
        glBufferStorage(GL_UNIFORM_BUFFER, sizeof(MATRICES), nullptr, GL_MAP_WRITE_BIT);

        glGenBuffers(1, &m_texture_handle_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_texture_handle_buffer);
        glBufferStorage(GL_UNIFORM_BUFFER, NUM_TEXTURES * sizeof(GLuint64) * 2, nullptr, GL_MAP_WRITE_BIT);

        GLuint64* pHandles = (GLuint64*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, NUM_TEXTURES * sizeof(GLuint64) * 2, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        for (int i = 0; i < NUM_TEXTURES; ++i) {
            unsigned int r = (get_random_uint(m_random) & 0xFCFF3F) << (get_random_uint(m_random) % 12);
            glGenTextures(1, &textures[i].name);
            glBindTexture(GL_TEXTURE_2D, textures[i].name);
            glTexStorage2D(GL_TEXTURE_2D, TEXTURE_LEVELS, GL_RGBA8, TEXTURE_SIZE, TEXTURE_SIZE);
            for (int j = 0; j < 32 * 32; ++j) {
                mutated_data[j] = (((unsigned int*)tex_data)[j] & r) | 0x20202020;
            }
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEXTURE_SIZE, TEXTURE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, mutated_data);
            glGenerateMipmap(GL_TEXTURE_2D);
            textures[i].handle = glGetTextureHandleARB(textures[i].name);
            glMakeTextureHandleResidentARB(textures[i].handle);
            pHandles[i * 2] = textures[i].handle;
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

        if (m_input_mode) {
            m_camera.OnUpdate(input, 3.0f, 0.2f, dt);
        }

        //Implement Camera Movement Functions
        if (input.Pressed(GLFW_KEY_LEFT_CONTROL)) {
            m_input_mode = !m_input_mode;
            input.SetRawMouseMode(window.GetHandle(), m_input_mode);
        }
	}
	void OnDraw() {
        float f = (float)m_time * 0.01f;

		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

        glFinish();

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_transform_buffer);
        MATRICES* pMatrices = (MATRICES*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(MATRICES), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        pMatrices->view = m_camera.m_view;
        pMatrices->projection = m_camera.m_proj;

        float angle = f;
        float angle2 = 0.7f * f;
        float angle3 = 0.1f * f;
        for (int i = 0; i < NUM_TEXTURES; i++)
        {
            pMatrices->model[i] = glm::translate(glm::vec3(float(i % 32) * 4.0f - 62.0f, float(i >> 5) * 6.0f - 33.0f, 15.0f * sinf(angle * 0.19f) + 3.0f * cosf(angle2 * 6.26f) + 40.0f * sinf(angle3))) *
                glm::rotate(angle * 130.0f, glm::vec3(1.0f, 0.0f, 0.0f)) *
                glm::rotate(angle * 140.0f, glm::vec3(0.0f, 0.0f, 1.0f));
            angle += 1.0f;
            angle2 += 4.1f;
            angle3 += 0.01f;
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);

        glFinish();

        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_texture_handle_buffer);

        glEnable(GL_DEPTH_TEST);

        glUseProgram(m_program);

        m_cube.OnDraw(NUM_TEXTURES);
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
#endif //BINDLESS_TEXTURES