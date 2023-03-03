#pragma once

#include "GL/glew.h" 

#include <iostream>

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"
using namespace glm;

#include "tiny_gltf.h"

namespace SB
{
	struct Mesh {
		Mesh(const tinygltf::Model& model, int mesh_index);
		string m_name;
		GLuint m_vao;
		GLuint m_vertex_buffer;
		GLuint m_index_buffer;
		GLsizei m_count;

		void OnDraw();
	};

	Mesh::Mesh(const tinygltf::Model& model, int mesh_index)
		:m_name(model.meshes[mesh_index].name)
	{
		vector<float> positions;
		vector<float> normals;
		vector<float> texcoords;
		vector<int> indices;

		//Continue Here
		const auto& mesh = model.meshes[mesh_index];
		//Extract the Position, Normal and TextureCoord data for current mesh
		for (const auto& primitive : mesh.primitives) {
			//Get Positions
			const auto& positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
			const auto& positionView = model.bufferViews[positionAccessor.bufferView];
			const float* positionData = reinterpret_cast<const float*>(model.buffers[positionView.buffer].data.data() + positionView.byteOffset + positionAccessor.byteOffset);
			for (size_t i = 0; i < positionAccessor.count * 3; i++) {
				positions.push_back(positionData[i]);
			}

			//Get Normals
			if (primitive.attributes.count("NORMAL") > 0) {
				const auto& normalAccessor = model.accessors[primitive.attributes.at("NORMAL")];
				const auto& normalView = model.bufferViews[normalAccessor.bufferView];
				const float* normalData = reinterpret_cast<const float*>(model.buffers[normalView.buffer].data.data() + normalView.byteOffset + normalAccessor.byteOffset);
				for (size_t i = 0; i < normalAccessor.count * 3; i++) {
					normals.push_back(normalData[i]);
				}
			}

			//Get Texture Coords
			if (primitive.attributes.count("TEXCOORD_0") > 0) {
				const auto& texAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
				const auto& texView = model.bufferViews[texAccessor.bufferView];
				const float* texCoordData = reinterpret_cast<const float*>(model.buffers[texView.buffer].data.data() + texView.byteOffset + texAccessor.byteOffset);
				for (size_t i = 0; i < texAccessor.count * 2; i++) {
					texcoords.push_back(texCoordData[i]);
				}
			}

			//Get Indices
			const auto& indexAccessor = model.accessors[primitive.indices];
			const auto& indexView = model.bufferViews[indexAccessor.bufferView];
			const u16* indexData = reinterpret_cast<const u16*>(model.buffers[indexView.buffer].data.data() + indexView.byteOffset + indexAccessor.byteOffset);
			for (size_t i = 0; i < indexAccessor.count; i++) {
				indices.push_back(indexData[i]);
			}
		}
		//Rearrange data into a vector<float> of vertex data
		vector<float> vertex;
		for (size_t i = 0; i < positions.size() / 3; ++i) {
			vertex.push_back(positions[i + 0]);
			vertex.push_back(positions[i + 1]);
			vertex.push_back(positions[i + 2]);
			vertex.push_back(normals[i + 0]);
			vertex.push_back(normals[i + 1]);
			vertex.push_back(normals[i + 2]);
			vertex.push_back(texcoords[i + 0]);
			vertex.push_back(texcoords[i + 1]);
		}
		m_count = indices.size();

		//Bind all data to related VAO
		glCreateVertexArrays(1, &m_vao);

		glCreateBuffers(1, &m_vertex_buffer);
		glNamedBufferStorage(m_vertex_buffer, vertex.size() * sizeof(float), vertex.data(), 0);

		glCreateBuffers(1, &m_index_buffer);
		glNamedBufferStorage(m_index_buffer, indices.size() * sizeof(int), indices.data(), 0);

		glVertexArrayAttribBinding(m_vao, 0, 0);
		glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glEnableVertexArrayAttrib(m_vao, 0);

		glVertexArrayAttribBinding(m_vao, 1, 0);
		glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
		glEnableVertexArrayAttrib(m_vao, 1);

		glVertexArrayAttribBinding(m_vao, 2, 0);
		glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
		glEnableVertexArrayAttrib(m_vao, 2);

		glVertexArrayVertexBuffer(m_vao, 0, m_vertex_buffer, 0, sizeof(float) * 8);
		glVertexArrayElementBuffer(m_vao, m_index_buffer);

		glBindVertexArray(0);
		//Create update and draw calls to render our scene data
		std::cout << "hi" << std::endl;
	}

	void Mesh::OnDraw() {
		glBindVertexArray(m_vao);
		glDrawElements(GL_TRIANGLES, m_count, GL_UNSIGNED_INT, (void*)0);
	}

	struct Node {
		Node(const tinygltf::Model& model, int current_node);
		string m_name;
		vec3 m_translation;
		vec4 m_rotation;
		vec3 m_scale;
		mat4 m_trs_matrix;
		Mesh m_mesh;

		void OnDraw();
	};

	Node::Node(const tinygltf::Model& model, int current_node)
		:m_name(model.nodes[current_node].name), 
		m_translation(vec3(0.0f)), 
		m_rotation(vec4(0.0f)), 
		m_scale(vec3(1.0f)), 
		m_mesh(model, model.nodes[current_node].mesh)
	{
		tinygltf::Node node = model.nodes[current_node];
		if (node.translation.size() != 0) {
			m_translation = vec3((float)node.translation[0], node.translation[1], node.translation[2]);
		}
		if (node.rotation.size() != 0) {
			m_rotation = vec4((float)node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
		}
		if (node.scale.size() != 0) {
			m_scale = vec3((float)node.scale[0], node.scale[1], node.scale[2]);
		}

		glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), m_translation);
		glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0f), m_rotation.w, glm::vec3(m_rotation));
		glm::mat4 scaling_matrix = glm::scale(glm::mat4(1.0f), m_scale);

		std::cout << glm::to_string(translation_matrix) << std::endl;
		std::cout << glm::to_string(rotation_matrix) << std::endl;
		std::cout << glm::to_string(scaling_matrix) << std::endl;

		m_trs_matrix = translation_matrix * rotation_matrix * scaling_matrix;
	}

	void Node::OnDraw() {
		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(m_trs_matrix));
		m_mesh.OnDraw();
	}

	struct Scene {
		Scene(const tinygltf::Model& model, int current_scene);
		string m_name;
		vector<Node> m_nodes;

		void OnDraw();
	};

	Scene::Scene(const tinygltf::Model& model, int current_scene)
		:m_name(model.scenes[current_scene].name)
	{
		for (const auto node : model.scenes[current_scene].nodes) {
			m_nodes.push_back(Node(model, node));
		}
	}

	void Scene::OnDraw() {
		for (Node& node : m_nodes) {
			node.OnDraw();
		}
	}

	struct Model {
		Model();
		Model(const char* filename);
		string m_filename;
		int m_default_scene;
		int m_current_scene;
		vector<Scene> m_scenes;

		void OnUpdate(f64 dt);
		void OnDraw();
	};

	Model::Model()
		:m_filename(""),
		m_default_scene(0),
		m_current_scene(0)
	{}

	Model::Model(const char* filename)
		:m_filename(filename)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
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

		m_default_scene = model.defaultScene;
		m_current_scene = model.defaultScene;

		
		for (size_t i = 0; i < model.scenes.size(); ++i) {
			m_scenes.push_back(Scene(model, i));
		}
	}

	void Model::OnUpdate(f64 dt) {

	}

	void Model::OnDraw() {
		for (Scene& scene : m_scenes) {
			scene.OnDraw();
		}
	}
}





