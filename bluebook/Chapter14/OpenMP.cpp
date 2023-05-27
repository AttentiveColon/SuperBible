#include "Defines.h"
#ifdef OPENMP
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include <chrono>
#include <omp.h>

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {

		int max_threads = omp_get_max_threads();
		omp_set_num_threads(max_threads);

		auto start_time = std::chrono::steady_clock::now();
		for (int i = 0; i < 8; ++i) {
			u64 result = 0;
			for (int j = 0; j < INT32_MAX; ++j) {
				if (j & 1) result += j;
				else result -= j;
			}
			printf("Iteration: %d  Thread: %d  Result: %ju\n", i, omp_get_thread_num(), result);
		}
		auto end_time = std::chrono::steady_clock::now();
		auto elapsted_time_no_threads = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
		std::cout << "Elapsed No Thread Load Time: " << elapsted_time_no_threads << " milliseconds" << std::endl;

		start_time = std::chrono::steady_clock::now();
		#pragma omp parallel for schedule (dynamic) //num_threads(max_threads)
		for (int i = 0; i < 8; ++i) {
			u64 result = 0;
			for (int j = 0; j < INT32_MAX; ++j) {
				if (j & 1) result += j;
				else result -= j;
			}
			printf("Iteration: %d  Thread: %d  Result: %ju\n", i, omp_get_thread_num(), result);
		}
		end_time = std::chrono::steady_clock::now();
		auto elapsted_time_threads = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
		std::cout << "Elapsed Thread Load Time: " << elapsted_time_threads << " milliseconds" << std::endl;
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
#endif //OPENMP