#pragma once

#include "GL/glew.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/matrix_transform.hpp"
using namespace glm;

#include "tiny_gltf.h"

struct Mesh {
	Mesh(const tinygltf::Model& model, int mesh_index);
	string m_name;
	GLuint m_vao;
	GLuint m_vertex_buffer;
	GLuint m_index_buffer;
	GLsizei m_count;
};

Mesh::Mesh(const tinygltf::Model& model, int mesh_index) 
	:m_name(model.meshes[mesh_index].name)
{
	//Continue Here
	//Extract the Position, Normal and TextureCoord data for current mesh
	//Rearrange data into a vector<float> of vertex data
	//Extract the index data for current mesh
	//Create vector<int> of index data

	//Bind all data to related VAO
	//Create update and draw calls to render our scene data

}

struct Node {
	Node(const tinygltf::Model& model, int current_node);
	string m_name;
	Mesh m_mesh;
	vec3 m_translation;
	vec4 m_rotation;
	vec3 m_scale;
	mat4 m_trs_matrix;
};

Node::Node(const tinygltf::Model& model, int current_node)
	:m_name(model.nodes[current_node].name), m_translation(vec3(0.0f)), m_rotation(vec4(0.0f)), m_scale(vec3(0.0f))
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
	
	m_trs_matrix = translation_matrix * rotation_matrix * scaling_matrix;

	m_mesh = Mesh(model, node.mesh);
}

struct Scene {
	Scene(const tinygltf::Model& model, int current_scene);
	string m_name;
	vector<Node> m_nodes;
};

Scene::Scene(const tinygltf::Model& model, int current_scene)
	:m_name(model.scenes[current_scene].name)
{
	for (const auto node : model.scenes[current_scene].nodes) {
		m_nodes.push_back(Node(model, node));
	}
}

struct Model {
	Model(const char* filename);
	string m_filename;
	int m_default_scene;
	int m_current_scene;
	vector<Scene> m_scenes;

	void OnUpdate(f64 dt);
	void OnDraw();
};

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

	int current_scene = 0;
	for (const auto& scene : model.scenes) {
		m_scenes.push_back(Scene(model, current_scene++));
	}
}





