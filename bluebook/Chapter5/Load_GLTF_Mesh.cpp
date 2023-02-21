#include "Defines.h"
#ifdef LOAD_GLTF_MESH
#include "System.h"
#include "tiny_gltf.h"
#include <string>
#include <vector>

using namespace tinygltf;

struct GLTFMeshData {
	std::vector<f32> positions;
	std::vector<f32> normals;
	std::vector<f32> texcoords;
	std::vector<i32> indices;
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	Application()
		:m_clear_color{ 0.0f, 0.0f, 0.0f, 1.0f },
		m_fps(0),
		m_time(0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		audio.PlayOneShot("./resources/startup.mp3");

		GLTFMeshData meshData;

		Model model;
		TinyGLTF loader;
		std::string err;
		std::string warn;

		bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, "./resources/two_planes.glb");

		if (!warn.empty()) {
			printf("Warn: %s\n", warn.c_str());
		}

		if (!err.empty()) {
			printf("Err: %s\n", err.c_str());
		}

		if (!ret) {
			printf("Failed to parse glTF\n");
			assert(false);
		}

		

		for (const auto& mesh : model.meshes) {
			for (const auto& primitive : mesh.primitives) {
				//Get Vertex Positions
				const Accessor& positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
				const BufferView& positionView = model.bufferViews[positionAccessor.bufferView];
				const float* positionData = reinterpret_cast<const float*>(model.buffers[positionView.buffer].data.data() + positionView.byteOffset + positionAccessor.byteOffset);
				for (usize i = 0; i < positionAccessor.count * 3; i++) {
					meshData.positions.push_back(positionData[i]);
				}

				//Get Normals
				if (primitive.attributes.count("NORMAL") > 0) {
					const Accessor& normalAccessor = model.accessors[primitive.attributes.at("NORMAL")];
					const BufferView& normalView = model.bufferViews[normalAccessor.bufferView];
					const float* normalData = reinterpret_cast<const float*>(model.buffers[normalView.buffer].data.data() + normalView.byteOffset + normalAccessor.byteOffset);
					for (usize i = 0; i < normalAccessor.count * 3; i++) {
						meshData.normals.push_back(normalData[i]);
					}
				}

				//Get Texture Coords
				if (primitive.attributes.count("TEXCOORD_0") > 0) {
					const Accessor& texAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
					const BufferView& texView = model.bufferViews[texAccessor.bufferView];
					const float* texCoordData = reinterpret_cast<const float*>(model.buffers[texView.buffer].data.data() + texView.byteOffset + texAccessor.byteOffset);
					for (usize i = 0; i < texAccessor.count * 3; i++) {
						meshData.texcoords.push_back(texCoordData[i]);
					}
				}

				//Get Indices
				const Accessor& indexAccessor = model.accessors[primitive.indices];
				const BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
				const u16* indexData = reinterpret_cast<const u16*>(model.buffers[indexView.buffer].data.data() + indexView.byteOffset + indexAccessor.byteOffset);
				for (usize i = 0; i < indexAccessor.count; i++) {
					meshData.indices.push_back(indexData[i]);
				}

				
			}
		}
		std::cout << "ALL DONE" << std::endl;

	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
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
#endif //TEST
