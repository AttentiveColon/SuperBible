#pragma once

#include "GL/glew.h" 

#include <iostream>

#include <string>
#include <vector>
using std::string;
using std::vector;

#include <filesystem>
using std::filesystem::path;

#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
using namespace glm;

#include "tiny_gltf.h"

namespace SB
{
	enum CameraType {
		Perspective,
		Orthographic
	};

	struct CameraDescriptor {
		string name;

		glm::vec3 translation;
		glm::quat rotation;

		CameraType type;
		double aspect_or_xmag;
		double fovy_or_ymag;
		double znear;
		double zfar;
	};

	struct Camera {
		Camera();
		Camera(CameraDescriptor camera);
		Camera(string name, glm::vec3 eye, glm::vec3 center, CameraType type, double aspect_or_xmag, double fovy_or_ymag, double znear, double zfar);

		glm::mat4 ViewProj() { return m_viewproj; }

		string m_name;
		glm::vec3 m_cam_position;
		glm::vec3 m_forward_vector;
		glm::quat m_rotation;

		glm::mat4 m_proj;
		glm::mat4 m_view;
		glm::mat4 m_viewproj;

		void OnUpdate(Input& input, float speed, double dt);
	};

	Camera::Camera() {

	}

	Camera::Camera(CameraDescriptor camera) 
		:m_name(camera.name),
		m_cam_position(camera.translation),
		m_rotation(camera.rotation)
	{
		m_forward_vector = glm::rotate(m_rotation, glm::vec3(0.0f, 0.0f, -1.0f));
		m_view = glm::lookAt(m_cam_position, m_forward_vector, glm::vec3(0.0f, 1.0f, 0.0f));
		if (camera.type == CameraType::Perspective) {
			m_proj = glm::perspective(camera.fovy_or_ymag, camera.aspect_or_xmag, camera.znear, camera.zfar);
		}
		else {
			m_proj = glm::ortho(camera.aspect_or_xmag, camera.fovy_or_ymag, camera.znear, camera.zfar);
		}
		m_viewproj = m_proj * m_view;
	}

	Camera::Camera(string name, glm::vec3 eye, glm::vec3 center, CameraType type, double aspect_or_xmag, double fovy_or_ymag, double znear, double zfar) 
		:m_name(name),
		m_cam_position(eye)
	{
		m_forward_vector = glm::normalize(center - eye);
		m_view = glm::lookAt(eye, m_forward_vector, glm::vec3(0.0f, 1.0f, 0.0));
		m_rotation = glm::conjugate(glm::toQuat(m_view));
		if (type == CameraType::Perspective) {
			m_proj = glm::perspective(fovy_or_ymag, aspect_or_xmag, znear, zfar);
		}
		else {
			m_proj = glm::ortho(aspect_or_xmag, fovy_or_ymag, znear, zfar);
		}
		m_viewproj = m_proj * m_view;
	}

	void Camera::OnUpdate(Input& input, float speed, double dt) {
		if (input.Held(GLFW_KEY_W)) {
			glm::vec3 translation_vector = -m_forward_vector * (float)dt * speed;
			m_view *= glm::translate(glm::mat4(1.0f), translation_vector);
		}
		if (input.Held(GLFW_KEY_S)) {
			glm::vec3 translation_vector = m_forward_vector * (float)dt * speed;
			m_view *= glm::translate(glm::mat4(1.0f), translation_vector);
 		}
		if (input.Held(GLFW_KEY_A)) {
			glm::vec3 translation_vector = glm::rotate(m_rotation, glm::vec3(1.0f, 0.0f, 0.0f)) * (float)dt * speed;
			m_view *= glm::translate(glm::mat4(1.0f), translation_vector);
		}
		if (input.Held(GLFW_KEY_D)) {
			glm::vec3 translation_vector = glm::rotate(m_rotation, glm::vec3(-1.0f, 0.0f, 0.0f)) * (float)dt * speed;
			m_view *= glm::translate(glm::mat4(1.0f), translation_vector);
		}
		if (input.Held(GLFW_KEY_Q)) {
			glm::vec3 translation_vector = glm::vec3(0.0f, 1.0f, 0.0f) * (float)dt * speed;
			m_view *= glm::translate(glm::mat4(1.0f), translation_vector);
		}
		if (input.Held(GLFW_KEY_E)) {
			glm::vec3 translation_vector = glm::vec3(0.0f, -1.0f, 0.0f) * (float)dt * speed;
			m_view *= glm::translate(glm::mat4(1.0f), translation_vector);
		}

		m_cam_position = glm::vec3(m_view[3][0], m_view[3][1], m_view[3][2]);
		m_forward_vector = glm::rotate(m_rotation, glm::vec3(0.0f, 0.0f, -1.0f));
		m_viewproj = m_proj * m_view;
	}

	struct Mesh {
		Mesh(const tinygltf::Model& model, int mesh_index);
		string m_name;
		int m_mesh_index;
		GLuint m_vao = 0;
		GLuint m_vertex_buffer = 0;
		GLuint m_index_buffer = 0;
		GLsizei m_count = 0;

		void OnDraw();
	};

	Mesh::Mesh(const tinygltf::Model& model, int mesh_index)
		:m_name(model.meshes[mesh_index].name),
		m_mesh_index(mesh_index)
	{
		vector<float> positions;
		vector<float> normals;
		vector<float> texcoords;
		vector<unsigned int> indices;

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
			if (indexAccessor.componentType == GL_UNSIGNED_INT) {
				const unsigned int* indexData = reinterpret_cast<const unsigned int*>(model.buffers[indexView.buffer].data.data() + indexView.byteOffset + indexAccessor.byteOffset);
				for (size_t i = 0; i < indexAccessor.count; i++) {
					indices.push_back(indexData[i]);
				}
			}
			else {
				const unsigned short* indexData = reinterpret_cast<const unsigned short*>(model.buffers[indexView.buffer].data.data() + indexView.byteOffset + indexAccessor.byteOffset);
				for (size_t i = 0; i < indexAccessor.count; i++) {
					indices.push_back(indexData[i]);
				}
			}
		}
		//Rearrange data into a vector<float> of vertex data
		vector<float> vertex;
		for (size_t i = 0; i < positions.size() / 3; ++i) {
			vertex.push_back(positions[3 * i + 0]);
			vertex.push_back(positions[3 * i + 1]);
			vertex.push_back(positions[3 * i + 2]);
			vertex.push_back(normals[3 * i + 0]);
			vertex.push_back(normals[3 * i + 1]);
			vertex.push_back(normals[3 * i + 2]);
			vertex.push_back(texcoords[2 * i + 0]);
			vertex.push_back(texcoords[2 * i + 1]);
		}
		m_count = indices.size();

		//Bind all data to related VAO
		glCreateVertexArrays(1, &m_vao);

		glCreateBuffers(1, &m_vertex_buffer);
		glNamedBufferStorage(m_vertex_buffer, vertex.size() * sizeof(float), &vertex[0], 0);

		glCreateBuffers(1, &m_index_buffer);
		glNamedBufferStorage(m_index_buffer, indices.size() * sizeof(unsigned int), &indices[0], 0);

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
	}

	void Mesh::OnDraw() {
		glBindVertexArray(m_vao);
		glDrawElements(GL_TRIANGLES, m_count, GL_UNSIGNED_INT, (void*)0);
	}

	struct Node {
		Node(const tinygltf::Node& node, int current_node);
		string m_name;
		int m_node_index;
		vec3 m_translation;
		quat m_rotation;
		vec3 m_scale;
		mat4 m_trs_matrix;

		int m_mesh_index;
		vector<int> m_children_nodes;
	};

	Node::Node(const tinygltf::Node& node, int current_node)
		:m_name(node.name),
		m_node_index(current_node),
		m_translation(vec3(0.0f)),
		m_rotation(quat(0.0f, 0.0, 0.0, 0.0)),
		m_scale(vec3(1.0f)),
		m_mesh_index(node.mesh)
	{
		if (node.translation.size() != 0) {
			m_translation = vec3((float)node.translation[0], node.translation[1], node.translation[2]);
		}
		if (node.rotation.size() != 0) {
			m_rotation = quat((float)node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
		}
		if (node.scale.size() != 0) {
			m_scale = vec3((float)node.scale[0], node.scale[1], node.scale[2]);
		}

		glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), m_translation);
		glm::mat4 rotation_matrix = glm::mat4_cast(m_rotation);
		glm::mat4 scaling_matrix = glm::scale(glm::mat4(1.0f), m_scale);
		m_trs_matrix = translation_matrix * rotation_matrix * scaling_matrix;

		for (size_t i = 0; i < node.children.size(); ++i) {
			m_children_nodes.push_back(node.children[i]);
		}
	}

	struct Scene {
		Scene(const tinygltf::Scene& scene, int current_scene);
		string m_name;
		int m_scene_index;
		vector<int> m_node_indices;
	};

	Scene::Scene(const tinygltf::Scene& scene, int current_scene)
		:m_name(scene.name),
		m_scene_index(current_scene)
	{
		for (const auto node_index : scene.nodes) {
			m_node_indices.push_back(node_index);
		}
	}

	struct Model {
		Model();
		Model(const char* filename);
		string m_filename;
		int m_default_scene;
		int m_current_scene;
		int m_current_camera;

		vector<Scene> m_scenes;
		vector<Node> m_nodes;
		vector<Mesh> m_meshes;
		vector<Camera> m_cameras;

		void DrawNode(glm::mat4 trs_matrix, int node_index);
		Camera GetCamera(int index);
		Camera GetNextCamera();

		void OnUpdate(f64 dt);
		void OnDraw();
	};

	Model::Model()
		:m_filename(""),
		m_default_scene(0),
		m_current_scene(0),
		m_current_camera(0)
	{}

	Model::Model(const char* filename)
		:m_filename(filename)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;
		bool ret;

		path filepath = filename;

		if (filepath.extension() == ".glb") {
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
		}
		else {
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
		}


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
			m_scenes.push_back(Scene(model.scenes[i], i));
		}
		for (size_t i = 0; i < model.nodes.size(); ++i) {
			m_nodes.push_back(Node(model.nodes[i], i));
		}
		for (size_t i = 0; i < model.meshes.size(); ++i) {
			m_meshes.push_back(Mesh(model, i));
		}

		//Cameras
		for (size_t i = 0; i < model.nodes.size(); ++i) {
			if (model.nodes[i].camera >= 0) {
				tinygltf::Node& curr_node = model.nodes[i];
				CameraDescriptor cam_desc;
				cam_desc.name = curr_node.name;
				cam_desc.translation = glm::vec3(curr_node.translation[0], curr_node.translation[1], curr_node.translation[2]);
				cam_desc.rotation = glm::quat(curr_node.rotation[3], curr_node.rotation[0], curr_node.rotation[1], curr_node.rotation[2]);

				tinygltf::Camera& curr_camera = model.cameras[curr_node.camera];
				if (curr_camera.type == "perspective") {
					cam_desc.type = CameraType::Perspective;
					cam_desc.aspect_or_xmag = curr_camera.perspective.aspectRatio;
					cam_desc.fovy_or_ymag = curr_camera.perspective.yfov;
					cam_desc.znear = curr_camera.perspective.znear;
					cam_desc.zfar = curr_camera.perspective.zfar;
				}
				else {
					cam_desc.type = CameraType::Perspective;
					cam_desc.aspect_or_xmag = curr_camera.orthographic.xmag;
					cam_desc.fovy_or_ymag = curr_camera.orthographic.ymag;
					cam_desc.znear = curr_camera.orthographic.znear;
					cam_desc.zfar = curr_camera.orthographic.zfar;
				}
				m_cameras.push_back(Camera(cam_desc));
			}
		}
	}

	void Model::DrawNode(glm::mat4 trs_matrix, int node_index) {
		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(trs_matrix));
		if (m_nodes[node_index].m_mesh_index >= 0) {
			m_meshes[m_nodes[node_index].m_mesh_index].OnDraw();
		}

		for (const auto& child_node_index : m_nodes[node_index].m_children_nodes) {
			glm::mat4 new_trs_matrix = trs_matrix * m_nodes[child_node_index].m_trs_matrix;
			DrawNode(new_trs_matrix, child_node_index);
		}
	}

	Camera Model::GetCamera(int index) {
		return m_cameras[index];
	}

	Camera Model::GetNextCamera() {
		++m_current_camera;
		if (m_current_camera >= m_cameras.size()) {
			m_current_camera = 0;
		}
		return GetCamera(m_current_camera);
	}

	void Model::OnUpdate(f64 dt) {

	}

	void Model::OnDraw() {
		for (const auto& node_index : m_scenes[m_current_scene].m_node_indices) {
			DrawNode(m_nodes[node_index].m_trs_matrix, node_index);
		}
	}
}



