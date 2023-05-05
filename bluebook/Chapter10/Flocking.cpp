#include "Defines.h"
#ifdef FLOCKING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* compute_shader_source = R"(
#version 450 core

layout (local_size_x = 256) in;

layout (location = 5)
uniform float closest_allowed_dist = 50.0;
layout (location = 6)
uniform float rule1_weight = 0.18;
layout (location = 7)
uniform float rule2_weight = 0.05;
layout (location = 8)
uniform float rule3_weight = 0.17;
layout (location = 9)
uniform float rule4_weight = 0.02;


layout (location = 11)
uniform vec3 goal = vec3(0.0);

uniform float timestep = 0.4;

struct flock_member
{
    vec3 position;
    vec3 velocity;
};

layout (std430, binding = 0) readonly buffer members_in
{
    flock_member member[];
} input_data;

layout (std430, binding = 1) buffer members_out
{
    flock_member member[];
} output_data;

shared flock_member shared_member[gl_WorkGroupSize.x];

vec3 rule1(vec3 my_position, vec3 my_velocity, vec3 their_position, vec3 their_velocity)
{
    vec3 d = my_position - their_position;
    if (dot(d, d) < closest_allowed_dist)
        return d;
    return vec3(0.0);
}

vec3 rule2(vec3 my_position, vec3 my_velocity, vec3 their_position, vec3 their_velocity)
{
     vec3 d = their_position - my_position;
     vec3 dv = their_velocity - my_velocity;
     return dv / (dot(d, d) + 10.0);
}

void main(void)
{
    uint i, j;
    int global_id = int(gl_GlobalInvocationID.x);
    int local_id  = int(gl_LocalInvocationID.x);

    flock_member me = input_data.member[global_id];
    flock_member new_me;
    vec3 accelleration = vec3(0.0);
    vec3 flock_center = vec3(0.0);

    for (i = 0; i < gl_NumWorkGroups.x; i++)
    {
        flock_member them =
            input_data.member[i * gl_WorkGroupSize.x +
                              local_id];
        shared_member[local_id] = them;
        memoryBarrierShared();
        barrier();
        for (j = 0; j < gl_WorkGroupSize.x; j++)
        {
            them = shared_member[j];
            flock_center += them.position;
            if (i * gl_WorkGroupSize.x + j != global_id)
            {
                accelleration += rule1(me.position,
                                       me.velocity,
                                       them.position,
                                       them.velocity) * rule1_weight;
                accelleration += rule2(me.position,
                                       me.velocity,
                                       them.position,
                                       them.velocity) * rule2_weight;
            }
        }
        barrier();
    }

    flock_center /= float(gl_NumWorkGroups.x * gl_WorkGroupSize.x);
    new_me.position = me.position + me.velocity * timestep;
    accelleration += normalize(goal - me.position) * rule3_weight;
    accelleration += normalize(flock_center - me.position) * rule4_weight;
    new_me.velocity = me.velocity + accelleration * timestep;
    if (length(new_me.velocity) > 10.0)
        new_me.velocity = normalize(new_me.velocity) * 10.0;
    new_me.velocity = mix(me.velocity, new_me.velocity, 0.4);
    output_data.member[global_id] = new_me;
}
)";

static ShaderText compute_shader_text[] = {
	{GL_COMPUTE_SHADER, compute_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* vertex_shader_source = R"(
#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (location = 2) in vec3 bird_position;
layout (location = 3) in vec3 bird_velocity;

out VS_OUT
{
    flat vec3 color;
} vs_out;

layout (location = 4)
uniform mat4 mvp;

mat4 make_lookat(vec3 forward, vec3 up)
{
    vec3 side = cross(forward, up);
    vec3 u_frame = cross(side, forward);

    return mat4(vec4(side, 0.0),
                vec4(u_frame, 0.0),
                vec4(forward, 0.0),
                vec4(0.0, 0.0, 0.0, 1.0));
}

vec3 choose_color(float f)
{
    float R = sin(f * 6.2831853);
    float G = sin((f + 0.3333) * 6.2831853);
    float B = sin((f + 0.6666) * 6.2831853);

    return vec3(R, G, B) * 0.25 + vec3(0.75);
}

void main(void)
{
    mat4 lookat = make_lookat(normalize(bird_velocity), vec3(0.0, 1.0, 0.0));
    vec4 obj_coord = lookat * vec4(position.xyz, 1.0);
    gl_Position = mvp * (obj_coord + vec4(bird_position, 0.0));

    vec3 N = mat3(lookat) * normal;
    vec3 C = choose_color(fract(float(gl_InstanceID / float(1237.0))));

    vs_out.color = mix(C * 0.2, C, smoothstep(0.0, 0.8, abs(N).z));
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

layout (location = 0) out vec4 color;

in VS_OUT
{
    flat vec3 color;
} fs_in;

void main(void)
{
    color = vec4(fs_in.color.rgb, 1.0);
}
)";

static ShaderText render_shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const glm::vec3 geometry[] =
{
	// Positions
	glm::vec3(-5.0f, 1.0f, 0.0f),
	glm::vec3(-1.0f, 1.5f, 0.0f),
	glm::vec3(-1.0f, 1.5f, 7.0f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 0.0f, 10.0f),
	glm::vec3(1.0f, 1.5f, 0.0f),
	glm::vec3(1.0f, 1.5f, 7.0f),
	glm::vec3(5.0f, 1.0f, 0.0f),

	// Normals
	glm::vec3(0.0f),
	glm::vec3(0.0f),
	glm::vec3(0.107f, -0.859f, 0.00f),
	glm::vec3(0.832f, 0.554f, 0.00f),
	glm::vec3(-0.59f, -0.395f, 0.00f),
	glm::vec3(-0.832f, 0.554f, 0.00f),
	glm::vec3(0.295f, -0.196f, 0.00f),
	glm::vec3(0.124f, 0.992f, 0.00f),
};

enum
{
	WORKGROUP_SIZE = 256,
	NUM_WORKGROUPS = 64,
	FLOCK_SIZE = (NUM_WORKGROUPS * WORKGROUP_SIZE)
};

struct flock_member
{
	glm::vec3 position;
	unsigned int : 32;
	glm::vec3 velocity;
	unsigned int : 32;
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;
	Random m_random;

	GLuint m_flock_vao[2];
	GLuint m_flock_buffer[2];
	GLuint m_geometry_buffer;

	GLuint m_render_program, m_flocking_program;

	GLuint m_frame_index = 0;

	SB::Camera m_camera;
	bool m_input_mode = false;

	//tweakable uniforms
	float m_closest = 50.0f;
	float m_rule1 = 0.18f;
	float m_rule2 = 0.05f;
	float m_rule3 = 0.17f;
	float m_rule4 = 0.02f;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_random.Init();
		m_camera = SB::Camera("camera", glm::vec3(0.0f, 0.0f, -400.0f), glm::vec3(0.0), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_flocking_program = LoadShaders(compute_shader_text);
		m_render_program = LoadShaders(render_shader_text);

		glGenBuffers(2, m_flock_buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_flock_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, FLOCK_SIZE * sizeof(flock_member), NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_flock_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, FLOCK_SIZE * sizeof(flock_member), NULL, GL_DYNAMIC_COPY);

		int i;

		glGenBuffers(1, &m_geometry_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_geometry_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(geometry), geometry, GL_STATIC_DRAW);

		glGenVertexArrays(2, m_flock_vao);

		for (i = 0; i < 2; i++)
		{
			glBindVertexArray(m_flock_vao[i]);
			glBindBuffer(GL_ARRAY_BUFFER, m_geometry_buffer);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(8 * sizeof(glm::vec3)));

			glBindBuffer(GL_ARRAY_BUFFER, m_flock_buffer[i]);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(flock_member), NULL);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(flock_member), (void*)sizeof(glm::vec4));
			glVertexAttribDivisor(2, 1);
			glVertexAttribDivisor(3, 1);

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_flock_buffer[0]);
		flock_member* ptr = reinterpret_cast<flock_member*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, FLOCK_SIZE * sizeof(flock_member), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

		for (i = 0; i < FLOCK_SIZE; i++)
		{
			ptr[i].position = (glm::vec3(m_random.Float(), m_random.Float(), m_random.Float()) - glm::vec3(0.5f)) * 300.0f;
			ptr[i].velocity = (glm::vec3(m_random.Float(), m_random.Float(), m_random.Float()) - glm::vec3(0.5f));
		}

		glUnmapBuffer(GL_ARRAY_BUFFER);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
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
		float t = (float)m_time;
		static const float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		static const float one = 1.0f;

		glUseProgram(m_flocking_program);

		glm::vec3 goal = glm::vec3(sinf(t * 0.34f),
			cosf(t * 0.29f),
			sinf(t * 0.12f) * cosf(t * 0.5f));

		goal = goal * glm::vec3(135.0f, 125.0f, 160.0f);

		glUniform1f(5, m_closest);
		glUniform1f(6, m_rule1);
		glUniform1f(7, m_rule2);
		glUniform1f(8, m_rule3);
		glUniform1f(9, m_rule4);

		glUniform3fv(11, 1, glm::value_ptr(goal));

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_flock_buffer[m_frame_index]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_flock_buffer[m_frame_index ^ 1]);

		glDispatchCompute(NUM_WORKGROUPS, 1, 1);

		glViewport(0, 0, 1600, 900);
		glClearBufferfv(GL_COLOR, 0, black);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glUseProgram(m_render_program);

		glm::mat4 mvp = m_camera.ViewProj();

		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(mvp));

		glBindVertexArray(m_flock_vao[m_frame_index]);

		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 8, FLOCK_SIZE);

		m_frame_index ^= 1;
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat("Closest", &m_closest, 0.01f, 0.001f, 100.0f);
		ImGui::DragFloat("Rule 1", &m_rule1, 0.001f, 0.001f, 1.0f);
		ImGui::DragFloat("Rule 2", &m_rule2, 0.001f, 0.001f, 1.0f);
		ImGui::DragFloat("Rule 3", &m_rule3, 0.001f, 0.001f, 1.0f);
		ImGui::DragFloat("Rule 4", &m_rule4, 0.001f, 0.001f, 1.0f);
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
#endif //FLOCKING
