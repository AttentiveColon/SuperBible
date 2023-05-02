#include "Defines.h"
#ifdef PARALLEL_PREFIX_SUM
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* compute_shader_source = R"(
#version 450 core

layout (local_size_x = 1024) in;

layout (binding = 0) coherent buffer block1
{
	float input_data[gl_WorkGroupSize.x];
};

layout (binding = 1) coherent buffer block2
{
	float output_data[gl_WorkGroupSize.x];
};

shared float shared_data[gl_WorkGroupSize.x * 2];

void main()
{
	uint id = gl_LocalInvocationID.x;
	uint rd_id; 
	uint wr_id;
	uint mask;

	const uint steps = uint(log2(gl_WorkGroupSize.x)) + 1;
	uint step = 0;

	shared_data[id * 2] = input_data[id * 2];
	shared_data[id * 2 + 1] = input_data[id * 2 + 1];

	barrier();
	memoryBarrierShared();

	for (step = 0; step < steps; ++step)
	{
		mask = (1 << step) - 1;
		rd_id = ((id >> step) << (step + 1)) + mask;
		wr_id = rd_id + 1 + (id & mask);

		shared_data[wr_id] += shared_data[rd_id];

		barrier();
		memoryBarrierShared();
	}

	output_data[id * 2] = shared_data[id * 2];
	output_data[id * 2 + 1] = shared_data[id * 2 + 1];
}
)";

static ShaderText compute_shader_text[] = {
	{GL_COMPUTE_SHADER, compute_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

void PrintBuffers(float* buffer, int elements) {
	for (int i = 0; i < elements; ++i) {
		std::cout << buffer[i];
		if (i % 10 == 0 && i != 0) 
			std::cout << '\n';
		else 
			std::cout << ", ";
	}
	std::cout << "\n\n";
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_compute_shader;

	float input_buffer[2048];
	float output_buffer[2048];

	GLuint m_buffers[2];

	Random m_rand;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_rand.Init();
		m_compute_shader = LoadShaders(compute_shader_text);

		for (int i = 0; i < 2048; ++i) {
			input_buffer[i] = m_rand.Float();
			output_buffer[i] = 0.0f;
		}

		glGenBuffers(2, m_buffers);

		glUseProgram(m_compute_shader);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(input_buffer), input_buffer, GL_DYNAMIC_READ);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffers[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(output_buffer), nullptr, GL_DYNAMIC_DRAW);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffers[0]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffers[1]);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDispatchCompute(1, 1, 1);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		//PrintBuffers(input_buffer, 2048);
		
		float* buf = (float*)glMapNamedBufferRange(m_buffers[1], 0, sizeof(float) * 2048, GL_MAP_READ_BIT);
		for (int i = 0; i < 2048; ++i) {
			std::cout << buf[i] << ", ";
		}
		glUnmapNamedBuffer(m_buffers[1]);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);
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
#endif //PARALLEL_PREFIX_SUM
