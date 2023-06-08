#include "Defines.h"
#ifdef OMP_PARTICLES
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include <chrono>
#include <omp.h>

static const GLchar* vs_source = R"(
#version 450 core

layout (location = 0) in vec3 position;

out vec4 particle_color;

void main()
{
	particle_color = vec4(0.6, 0.8, 0.8, 1.0) * (smoothstep(-10.0, 10.0, position.z) * 0.6 + 0.4);
	gl_Position = vec4(position * 0.2, 1.0);
}
)";

static const GLchar* fs_source = R"(
#version 450 core

layout (location = 0) out vec4 color;

in vec4 particle_color;

void main()
{
	color = particle_color;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vs_source, NULL},
	{GL_FRAGMENT_SHADER, fs_source, NULL},
	{GL_NONE, NULL, NULL}
};

#define PARTICLE_COUNT 2048

struct Particle
{
	glm::vec3 position;
	glm::vec3 velocity;
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_vao;
	GLuint m_program;


	GLuint m_particle_buffer;
	Particle* m_mapped_buffer;
	Particle* m_particles[2];
	int m_frame;
	bool m_use_omp = true;

	Random m_random;
	int m_max_threads;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_max_threads = omp_get_max_threads();
		std::cout << m_max_threads << std::endl;
		omp_set_num_threads(m_max_threads);

		m_program = LoadShaders(shader_text);
		m_random.Init();

		InitBuffers();
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (m_use_omp) UpdateParticlesOMP(dt);
		else UpdateParticles(dt);
	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(m_vao);
		glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, PARTICLE_COUNT * sizeof(Particle));
		glPointSize(3.0f);
		glUseProgram(m_program);
		glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Use Threads", &m_use_omp);
		ImGui::End();
	}
	void InitBuffers() {
		m_particles[0] = new Particle[PARTICLE_COUNT];
		m_particles[1] = new Particle[PARTICLE_COUNT];

		glGenBuffers(1, &m_particle_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_particle_buffer);
		glBufferStorage(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(Particle), nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

		m_mapped_buffer = (Particle*)glMapBufferRange(GL_ARRAY_BUFFER, 0, PARTICLE_COUNT * sizeof(Particle), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

		InitParticles();

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

		glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
		glBindVertexBuffer(0, m_particle_buffer, 0, sizeof(Particle));
		glEnableVertexAttribArray(0);
	}
	void InitParticles() {
		for (int i = 0; i < PARTICLE_COUNT; ++i) {
			m_particles[0][i].position[0] = m_random.Float() * 6.0f - 3.0f;
			m_particles[0][i].position[1] = m_random.Float() * 6.0f - 3.0f;
			m_particles[0][i].position[2] = m_random.Float() * 6.0f - 3.0f;
			m_particles[0][i].velocity = m_particles[0][i].position * 0.001f;
			m_mapped_buffer[i] = m_particles[0][i];
		}
	}
	void UpdateParticles(f64 dt) {
		const Particle* const src = m_particles[m_frame & 1];
		Particle* const dst = m_particles[(m_frame + 1) & 1];

		for (int i = 0; i < PARTICLE_COUNT; ++i) {
			const Particle& me = src[i];
			glm::vec3 delta_v(0.0f);

			for (int j = 0; j < PARTICLE_COUNT; ++j) {
				if (i != j) {
					glm::vec3 delta_pos = src[j].position - me.position;
					float distance = glm::length(delta_pos);
					glm::vec3 delta_dir = delta_pos / distance;
					distance = distance < 0.005f ? 0.005f : distance;
					delta_v += (delta_dir / (distance * distance));
				} 
			}
			dst[i].position = me.position + me.velocity;
			dst[i].velocity = me.velocity + delta_v * (float)dt * 0.0001f;
			m_mapped_buffer[i].position = dst[i].position;
		}

		m_frame++;
	}
	void UpdateParticlesOMP(f64 dt) {
		const Particle* const src = m_particles[m_frame & 1];
		Particle* const dst = m_particles[(m_frame + 1) & 1];
#pragma omp parallel for schedule (static) num_threads(m_max_threads)
		for (int i = 0; i < PARTICLE_COUNT; ++i) {
			const Particle& me = src[i];
			glm::vec3 delta_v(0.0f);

			for (int j = 0; j < PARTICLE_COUNT; ++j) {
				if (i != j) {
					glm::vec3 delta_pos = src[j].position - me.position;
					float distance = glm::length(delta_pos);
					glm::vec3 delta_dir = delta_pos / distance;
					distance = distance < 0.005f ? 0.005f : distance;
					delta_v += (delta_dir / (distance * distance));
				}
			}
			dst[i].position = me.position + me.velocity;
			dst[i].velocity = me.velocity + delta_v * (float)dt * 0.0001f;
			m_mapped_buffer[i].position = dst[i].position;
		}

		m_frame++;
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
#endif //OMP_PARTICLES