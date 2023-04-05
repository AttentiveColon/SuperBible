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
#include "glm/gtx/rotate_vector.hpp"
using namespace glm;

#include "tiny_gltf.h"

#include "System.h"

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
		glm::vec3 Eye() { return glm::vec3(m_view[3][0], m_view[3][1], m_view[3][2]); }

		string m_name;
		glm::vec3 m_cam_position;
		glm::vec3 m_forward_vector;
		glm::quat m_rotation;

		glm::mat4 m_proj;
		glm::mat4 m_view;
		glm::mat4 m_viewproj;

		void OnUpdate(Input& input, float translation_speed, float rotation_speed, double dt);

	private:
		void Rotate(Input& input, float speed, double dt);
		void Translate(Input& input, float speed, double dt);
	};

	Camera::Camera() {

	}

	Camera::Camera(CameraDescriptor camera) 
		:m_name(camera.name),
		m_cam_position(camera.translation),
		m_rotation(camera.rotation)
	{
		m_forward_vector = -glm::rotate(m_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		m_view = glm::lookAt(m_cam_position, m_cam_position + m_forward_vector, glm::vec3(0.0f, 1.0f, 0.0f));
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

	void Camera::OnUpdate(Input& input, float translation_speed, float rotation_speed, double dt) {
		this->Rotate(input, rotation_speed, dt);
		this->Translate(input, translation_speed, dt);

		m_view = glm::lookAt(m_cam_position, m_cam_position + m_forward_vector, glm::vec3(0.0f, 1.0f, 0.0f));
		m_viewproj = m_proj * m_view;
	}

	void Camera::Rotate(Input& input, float speed, double dt) {
		MousePos mouse_delta = input.GetMouseRaw();
		m_forward_vector = glm::rotate(m_forward_vector, glm::radians((-(float)mouse_delta.x * (float)dt * speed)), glm::vec3(0.0f, 1.0f, 0.0f));
		m_forward_vector = glm::rotate(m_forward_vector, glm::radians((-(float)mouse_delta.y * (float)dt * speed)), glm::cross(m_forward_vector, glm::vec3(0.0f, 1.0f, 0.0f)));
	}

	void Camera::Translate(Input& input, float speed, double dt) {
		if (input.Held(GLFW_KEY_W)) {
			m_cam_position += m_forward_vector * (float)dt * speed;
		}
		if (input.Held(GLFW_KEY_S)) {
			m_cam_position -= m_forward_vector * (float)dt * speed;
		}
		if (input.Held(GLFW_KEY_A)) {
			m_cam_position -= glm::cross(m_forward_vector, glm::vec3(0.0f, 1.0f, 0.0f)) * (float)dt * speed;
		}
		if (input.Held(GLFW_KEY_D)) {
			m_cam_position += glm::cross(m_forward_vector, glm::vec3(0.0f, 1.0f, 0.0f)) * (float)dt * speed;
		}
		if (input.Held(GLFW_KEY_Q)) {
			m_cam_position += glm::cross(m_forward_vector, glm::cross(m_forward_vector, glm::vec3(0.0f, 1.0f, 0.0f))) * (float)dt * speed;
		}
		if (input.Held(GLFW_KEY_E)) {
			m_cam_position -= glm::cross(m_forward_vector, glm::cross(m_forward_vector, glm::vec3(0.0f, 1.0f, 0.0f))) * (float)dt * speed;
		}
	}

	struct Cameras {
		int m_current_cam = 0;
		void Init(tinygltf::Model& model);
		Camera& GetCamera(int index) { return m_cameras[index]; }
		Camera& GetNextCamera();
		vector<Camera> m_cameras;
	};

	Camera& Cameras::GetNextCamera() {
		if (m_cameras.size() != 0) {
			++m_current_cam;
			if (m_current_cam < m_cameras.size()) {
				return GetCamera(m_current_cam);
			}
			else {
				m_current_cam = 0;
				return GetCamera(m_current_cam);
			}
		}
		return GetCamera(m_current_cam);
	}

	void Cameras::Init(tinygltf::Model& model) {
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

	struct Images {
		Images() = default;
		void Init(vector<tinygltf::Image>& images);
		GLuint GetTexture(int index) { return m_textures[index]; }
		vector<GLuint> m_textures;
	};

	void Images::Init(vector<tinygltf::Image>& images) {
		m_textures.resize(images.size(), 0);
		glCreateTextures(GL_TEXTURE_2D, images.size(), m_textures.data());
		for (size_t i = 0; i < images.size(); ++i) {
			const auto& image = images[i];
			glTextureStorage2D(m_textures[i], 1, GL_RGBA32F, image.width, image.height);
			glBindTexture(GL_TEXTURE_2D, m_textures[i]);
			glTextureSubImage2D(m_textures[i], 0, 0, 0, image.width, image.height, GL_RGBA, image.pixel_type, image.image.data());
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		//Create single white pixel texture
		m_textures.push_back(0);
		glCreateTextures(GL_TEXTURE_2D, 1, &m_textures[m_textures.size() - 1]);
		glTextureStorage2D(m_textures[m_textures.size() - 1], 1, GL_RGBA32F, 1, 1);
		glBindTexture(GL_TEXTURE_2D, m_textures[m_textures.size() - 1]);
		int data = 0xFFFFFFFF;
		glTextureSubImage2D(m_textures[m_textures.size() - 1], 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
	}

	struct Sampler {
		void Init(vector<tinygltf::Sampler> samplers);
		GLuint GetSampler(int index) { return m_samplers[index]; }
		vector<GLuint> m_samplers;
	};

	void Sampler::Init(vector<tinygltf::Sampler> samplers) {
		m_samplers.resize(samplers.size(), 0);
		glCreateSamplers(samplers.size(), m_samplers.data());
		for (size_t i = 0; i < samplers.size(); ++i) {
			glSamplerParameteri(m_samplers[i], GL_TEXTURE_MAG_FILTER, samplers[i].magFilter);
			glSamplerParameteri(m_samplers[i], GL_TEXTURE_MIN_FILTER, samplers[i].minFilter);
			glSamplerParameteri(m_samplers[i], GL_TEXTURE_WRAP_S, samplers[i].wrapS);
			glSamplerParameteri(m_samplers[i], GL_TEXTURE_WRAP_T, samplers[i].wrapT);
		}
	}

	struct Material {
		Material(GLuint base_texture,
			GLuint metallic_roughness_texture,
			GLuint normal_texture,
			GLuint occlusion_texture,
			GLuint emissive_texture,
			GLuint base_sampler,
			GLuint metallic_sampler,
			GLuint normal_sampler,
			GLuint occlusion_sampler,
			GLuint emissive_sampler,
			const double* color_factor
		)
			:m_base_color_texture(base_texture),
			m_metallic_roughness_texture(metallic_roughness_texture),
			m_normal_texture(normal_texture),
			m_occlusion_texture(occlusion_texture),
			m_emissive_texture(emissive_texture),
			m_base_sampler(base_sampler),
			m_metallic_sampler(metallic_sampler),
			m_normal_sampler(normal_sampler),
			m_occlusion_sampler(occlusion_sampler),
			m_emissive_sampler(emissive_sampler),
			m_color_factors{ (float)color_factor[0], (float)color_factor[1], (float)color_factor[2], (float)color_factor[3] }
		{}

		GLuint m_base_color_texture;
		GLuint m_metallic_roughness_texture;
		GLuint m_normal_texture;
		GLuint m_occlusion_texture;
		GLuint m_emissive_texture;

		GLuint m_base_sampler;
		GLuint m_metallic_sampler;
		GLuint m_normal_sampler;
		GLuint m_occlusion_sampler;
		GLuint m_emissive_sampler;

		float m_color_factors[4];

		void BindMaterial();
	};

	void Material::BindMaterial() {
		if (m_base_color_texture >= 0) {
			glActiveTexture(GL_TEXTURE0 + 0);
			//glBindTexture(GL_TEXTURE_2D, m_base_color_texture);
			glBindTextureUnit(0, m_base_color_texture);
			glBindSampler(0, m_base_sampler);
		}
		if (m_metallic_roughness_texture >= 0) {
			glActiveTexture(GL_TEXTURE0 + 1);
			//glBindTexture(GL_TEXTURE_2D, m_metallic_roughness_texture);
			glBindTextureUnit(1, m_metallic_roughness_texture);
			glBindSampler(1, m_metallic_sampler);
		}
		if (m_normal_texture >= 0) {
			glActiveTexture(GL_TEXTURE0 + 2);
			//glBindTexture(GL_TEXTURE_2D, m_normal_texture);
			glBindTextureUnit(2, m_normal_texture);
			glBindSampler(2, m_normal_sampler);
		}
		if (m_occlusion_texture >= 0) {
			glActiveTexture(GL_TEXTURE0 + 3);
			//glBindTexture(GL_TEXTURE_2D, m_occlusion_texture);
			glBindTextureUnit(3, m_occlusion_texture);
			glBindSampler(3, m_occlusion_sampler);
		}
		if (m_emissive_texture >= 0) {
			glActiveTexture(GL_TEXTURE0 + 4);
			//glBindTexture(GL_TEXTURE_2D, m_emissive_texture);
			glBindTextureUnit(4, m_emissive_texture);
			glBindSampler(4, m_emissive_sampler);
		}
		glUniform4fv(7, 1, m_color_factors);
	}

	struct Materials {
		vector<Material> m_materials;
		void Init(tinygltf::Model& model, Images& images, Sampler& sampler);
		Material& GetMaterial(int index) { return m_materials[index]; }
	};

	void Materials::Init(tinygltf::Model& model, Images& images, Sampler& sampler) {
		for (size_t i = 0; i < model.materials.size(); ++i) {
			const auto& mat = model.materials[i];
			const auto& tex = model.textures;
			GLuint white_tex = images.GetTexture(images.m_textures.size() - 1);

			int base_texture_color = -1;
			int metallic_roughness_texture = -1;
			int normal_texture = -1;
			int occlusion_texture = -1;
			int emissive_texture = -1;

			int base_texture_sampler = 0;
			int metallic_roughness_sampler = 0;
			int normal_sampler = 0;
			int occlusion_sampler = 0;
			int emissive_sampler = 0;

			if (mat.pbrMetallicRoughness.baseColorTexture.index != -1) {
				int tex_index = tex[mat.pbrMetallicRoughness.baseColorTexture.index].source;
				if (tex_index != -1) {
					base_texture_color = images.GetTexture(tex_index);
					base_texture_sampler = sampler.GetSampler(tex[mat.pbrMetallicRoughness.baseColorTexture.index].sampler);
				}
			}
			if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
				int tex_index = tex[mat.pbrMetallicRoughness.metallicRoughnessTexture.index].source;
				if (tex_index != -1) {
					metallic_roughness_texture = images.GetTexture(tex_index);
					metallic_roughness_sampler = sampler.GetSampler(tex[mat.pbrMetallicRoughness.metallicRoughnessTexture.index].sampler);
				}
			}
			if (mat.normalTexture.index != -1) {
				int tex_index = tex[mat.normalTexture.index].source;
				if (tex_index != -1) {
					normal_texture = images.GetTexture(tex_index);
					normal_sampler = sampler.GetSampler(tex[mat.normalTexture.index].sampler);
				}
			}
			if (mat.occlusionTexture.index != -1) {
				int tex_index = tex[mat.occlusionTexture.index].source;
				if (tex_index != -1) {
					occlusion_texture = images.GetTexture(tex_index);
					occlusion_sampler = sampler.GetSampler(tex[mat.occlusionTexture.index].sampler);
				}
			}
			if (mat.emissiveTexture.index != -1) {
				int tex_index = tex[mat.emissiveTexture.index].source;
				if (tex_index != -1) {
					emissive_texture = images.GetTexture(tex_index);
					emissive_sampler = sampler.GetSampler(tex[mat.emissiveTexture.index].sampler);
				}
			}

			if (base_texture_color == -1) base_texture_color = white_tex;
			if (metallic_roughness_texture == -1) metallic_roughness_texture = white_tex;
			if (normal_texture == -1) normal_texture = white_tex;
			if (occlusion_texture == -1) occlusion_texture = white_tex;
			if (emissive_texture == -1) emissive_texture = white_tex;

			Material material = Material(base_texture_color,
				metallic_roughness_texture,
				normal_texture,
				occlusion_texture,
				emissive_texture,
				base_texture_sampler,
				metallic_roughness_sampler,
				normal_sampler,
				occlusion_sampler,
				emissive_sampler,
				mat.pbrMetallicRoughness.baseColorFactor.data()
			);
			m_materials.push_back(material);
		}
	}

	struct MeshData {
		GLuint m_vao;
		GLsizei m_count;
		GLint m_material;
		GLint m_topology;
	};

	struct Mesh {
		Mesh(const tinygltf::Model& model, int mesh_index);
		string m_name;
		vector<MeshData> m_meshes;
	};

	Mesh::Mesh(const tinygltf::Model& model, int mesh_index) 
		:m_name(model.meshes[mesh_index].name)
	{
		const auto& mesh = model.meshes[mesh_index];
		//Extract the Position, Normal and TextureCoord data for current mesh
		for (const auto& primitive : mesh.primitives) {

			GLuint m_vao, m_vertex_buffer, m_index_buffer;

			vector<float> positions;
			vector<float> normals;
			vector<float> texcoords;
			vector<unsigned int> indices;

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

			MeshData mesh_data;
			mesh_data.m_vao = m_vao;
			mesh_data.m_count = indices.size();
			mesh_data.m_material = primitive.material;
			mesh_data.m_topology = primitive.mode;
			
			m_meshes.push_back(mesh_data);
		}
		
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

		Images m_image;
		Materials m_material;
		Cameras m_camera;
		Sampler m_sampler;

		void DrawNode(glm::mat4 trs_matrix, int node_index);

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
		:m_filename(filename),
		m_default_scene(0),
		m_current_scene(0),
		m_current_camera(0)
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

		//Collect scenes
		for (size_t i = 0; i < model.scenes.size(); ++i) {
			m_scenes.push_back(Scene(model.scenes[i], i));
		}
		//Collect nodes
		for (size_t i = 0; i < model.nodes.size(); ++i) {
			m_nodes.push_back(Node(model.nodes[i], i));
		}
		//Collect meshes
		for (size_t i = 0; i < model.meshes.size(); ++i) {
			m_meshes.push_back(Mesh(model, i));
		}

		//Create Image Buffers
		m_image.Init(model.images);

		//Collect Samplers
		m_sampler.Init(model.samplers);

		//Collect Materials
		m_material.Init(model, m_image, m_sampler);

		//Collect cameras
		m_camera.Init(model);
	}

	void Model::DrawNode(glm::mat4 trs_matrix, int node_index) {
		glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(trs_matrix));
		if (m_nodes[node_index].m_mesh_index >= 0) {
			Mesh& mesh = m_meshes[m_nodes[node_index].m_mesh_index];
			for (int i = 0; i < mesh.m_meshes.size(); ++i) {
				glBindVertexArray(mesh.m_meshes[i].m_vao);
				if (mesh.m_meshes[i].m_material < m_material.m_materials.size()) {
					m_material.GetMaterial(mesh.m_meshes[i].m_material).BindMaterial();
				}
				else {
					//Fall back if no associated material
					// --- Needs lighting or a default texture other than white pixel to be visible
					const float color[] = { 0.5f, 0.5f, 0.5f, 0.5f };
					glActiveTexture(GL_TEXTURE0 + 0);
					glBindTextureUnit(0, m_image.GetTexture(m_image.m_textures.size() - 1));
					glUniform4fv(7, 1, color);
				}
				glDrawElements(mesh.m_meshes[i].m_topology, mesh.m_meshes[i].m_count, GL_UNSIGNED_INT, (void*)0);
			}
		}

		for (const auto& child_node_index : m_nodes[node_index].m_children_nodes) {
			glm::mat4 new_trs_matrix = trs_matrix * m_nodes[child_node_index].m_trs_matrix;
			DrawNode(new_trs_matrix, child_node_index);
		}
	}

	void Model::OnUpdate(f64 dt) {

	}

	void Model::OnDraw() {
		for (const auto& node_index : m_scenes[m_current_scene].m_node_indices) {
			DrawNode(m_nodes[node_index].m_trs_matrix, node_index);
		}
	}
}



