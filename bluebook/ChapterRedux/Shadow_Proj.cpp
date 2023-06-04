#include "Defines.h"
#ifdef SHADOW_PROJ
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <omp.h>

static const GLchar* shadow_map_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

layout (location = 0)
uniform mat4 u_model;
layout (location = 1)
uniform mat4 u_view;
layout (location = 2)
uniform mat4 u_proj;

void main()
{
	gl_Position = u_proj * u_view * u_model * vec4(position, 1.0);
}
)";

static const GLchar* shadow_map_fragment_shader_source = R"(
#version 450 core

void main()
{

}
)";

static ShaderText shadow_map_shader_text[] = {
	{GL_VERTEX_SHADER, shadow_map_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, shadow_map_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* blinn_phong_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec3 tangent;
layout (location = 3)
in vec3 bitangent;
layout (location = 4)
in vec2 texcoord;

layout (location = 0)
uniform mat4 u_model;
layout (location = 1)
uniform mat4 u_view;
layout (location = 2)
uniform mat4 u_proj;
layout (location = 3)
uniform vec3 u_cam_pos;
layout (location = 4)
uniform mat4 u_light_matrix;

layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient_albedo;
	float pad2;
	vec3 rim_color;
	float rim_power;
};

out VS_OUT
{
	vec3 FragPos;
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
	vec4 FragPosLightSpace;
} vs_out;

const mat4 bias = mat4(0.5, 0.0, 0.0, 0.0,
					   0.0, 0.5, 0.0, 0.0,
					   0.0, 0.0, 0.5, 0.0,
					   0.5, 0.5, 0.5, 1.0);

void main(void)
{

    vec4 P = u_model * vec4(position, 1.0);
	vs_out.FragPos = P.xyz;
	vs_out.N = mat3(u_model) * normal;
	vs_out.L = light_pos - P.xyz;
	vs_out.V = u_cam_pos - P.xyz;
	vs_out.uv = texcoord;
	vs_out.FragPosLightSpace = bias * u_light_matrix * P;

	gl_Position = u_proj * u_view * P;
}
)";

static const GLchar* blinn_phong_fragment_shader_source = R"(
#version 450 core

//layout  (binding = 1)
//uniform sampler2D u_diffuse;

layout (binding = 0)
uniform sampler2DShadow u_shadow;


layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient_albedo;
	float pad2;
	vec3 rim_color;
	float rim_power;
};

layout (location = 15)
uniform bool is_light;

in VS_OUT
{
	vec3 FragPos;
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
	vec4 FragPosLightSpace;
} fs_in;

out vec4 color;

void main()
{
	vec3 N = normalize(fs_in.N);
	vec3 L = normalize(fs_in.L);
	vec3 V = normalize(fs_in.V);

	vec3 H = normalize(L + V);

	//vec3 diffuse = max(dot(N, L), 0.0) * texture(u_diffuse, fs_in.uv).rgb;
	//vec3 ambient = texture(u_diffuse, fs_in.uv).rgb * ambient_albedo;
	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;
	vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;
	vec3 ambient = ambient_albedo;

	float bias = max(0.05 * (1.0 - dot(N, L)), 0.005);
	float shadow = textureProj(u_shadow, fs_in.FragPosLightSpace);
	
	color = vec4(((diffuse + specular) * shadow), 1.0) + vec4(ambient, 1.0);
	if (is_light) color = vec4(1.0);
}
)";

static ShaderText blinn_phong_shader_text[] = {
	{GL_VERTEX_SHADER, blinn_phong_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, blinn_phong_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Material_Uniform {
	glm::vec3 light_pos;
	float pad0;
	glm::vec3 diffuse_albedo;
	float pad1;
	glm::vec3 specular_albedo;
	float specular_power;
	glm::vec3 ambient;
	float pad2;
};

//laptop test push for contribution graph

struct Mesh
{
	GLuint vao;
	size_t count;
};

static void DrawMesh(const Mesh& mesh, glm::mat4 model) {
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
	glBindVertexArray(mesh.vao);
	glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, (void*)0);
}

Mesh ImportMesh(const char* filename) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs);

	if (nullptr == scene) {
		const char* error = importer.GetErrorString();
		printf(error);
		assert(false);
	}

	aiMesh* mesh = scene->mMeshes[0];
	aiVector3t<float>* vertices = mesh->mVertices;
	aiVector3t<float>* normals = mesh->mNormals;
	aiVector3t<float>* tangents = mesh->mTangents;
	aiVector3t<float>* bitangents = mesh->mBitangents;
	aiVector3t<float>* texcoords = mesh->mTextureCoords[0];

	vector<float> vertex_data;
	for (int i = 0; i < mesh->mNumVertices; ++i) {
		vertex_data.push_back(vertices[i].x);
		vertex_data.push_back(vertices[i].y);
		vertex_data.push_back(vertices[i].z);
		vertex_data.push_back(normals[i].x);
		vertex_data.push_back(normals[i].y);
		vertex_data.push_back(normals[i].z);
		vertex_data.push_back(tangents[i].x);
		vertex_data.push_back(tangents[i].y);
		vertex_data.push_back(tangents[i].z);
		vertex_data.push_back(bitangents[i].x);
		vertex_data.push_back(bitangents[i].y);
		vertex_data.push_back(bitangents[i].z);
		vertex_data.push_back(texcoords[i].x);
		vertex_data.push_back(texcoords[i].y);
	}

	aiFace* faces = mesh->mFaces;

	vector<unsigned int> index_data;
	for (int i = 0; i < mesh->mNumFaces; ++i) {
		index_data.push_back(faces[i].mIndices[0]);
		index_data.push_back(faces[i].mIndices[1]);
		index_data.push_back(faces[i].mIndices[2]);
	}

	GLuint vao, vertex_buffer, index_buffer;
	glCreateVertexArrays(1, &vao);

	glCreateBuffers(1, &vertex_buffer);
	glNamedBufferStorage(vertex_buffer, vertex_data.size() * sizeof(float), &vertex_data[0], 0);

	glCreateBuffers(1, &index_buffer);
	glNamedBufferStorage(index_buffer, index_data.size() * sizeof(unsigned int), &index_data[0], 0);

	//Position Vec3
	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(vao, 0);

	//Normal Vec3
	glVertexArrayAttribBinding(vao, 1, 0);
	glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
	glEnableVertexArrayAttrib(vao, 1);

	//Tangent Vec3
	glVertexArrayAttribBinding(vao, 2, 0);
	glVertexArrayAttribFormat(vao, 2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6);
	glEnableVertexArrayAttrib(vao, 2);

	//Bitangent Vec3
	glVertexArrayAttribBinding(vao, 3, 0);
	glVertexArrayAttribFormat(vao, 3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9);
	glEnableVertexArrayAttrib(vao, 3);

	//TexCoords Vec2
	glVertexArrayAttribBinding(vao, 4, 0);
	glVertexArrayAttribFormat(vao, 4, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 12);
	glEnableVertexArrayAttrib(vao, 4);

	glVertexArrayVertexBuffer(vao, 0, vertex_buffer, 0, sizeof(float) * 14);
	glVertexArrayElementBuffer(vao, index_buffer);

	glBindVertexArray(0);

	Mesh result;
	result.vao = vao;
	result.count = index_data.size();
	return result;
}

static const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_shadow_program;
	GLuint m_phong_program;


	GLuint m_ubo;
	Material_Uniform* m_data;

	glm::vec3 m_light_pos = glm::vec3(3.0f);
	glm::vec3 m_diffuse_albedo = glm::vec3(0.5f, 0.2f, 0.7f);
	glm::vec3 m_specular_albedo = glm::vec3(0.7f);
	float m_specular_power = 128.0f;
	glm::vec3 m_ambient = vec3(0.1f, 0.1f, 0.1f);

	glm::mat4 m_light_view;
	glm::mat4 m_light_proj;
	GLuint m_shadow_FBO;


	GLuint m_tex_base, m_tex_normal, m_tex_shadow;

	SB::Camera m_camera;
	bool m_input_mode = false;

	Random m_random;

	Mesh m_mesh[5];
	glm::mat4 m_mesh_model[5];

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		int num_threads = omp_get_max_threads();
		m_phong_program = LoadShaders(blinn_phong_shader_text);
		m_shadow_program = LoadShaders(shadow_map_shader_text);


		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 0.1f, 0.3f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);

		//m_light_proj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 70.5f);
		m_light_proj = glm::perspective(1.0, 1.0 / 1.0, 1.0, 1000.0);
		m_light_view = glm::lookAt(m_light_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_shadow_FBO = CreateShadowFrameBuffer();

		m_mesh[0] = ImportMesh("./resources/rook2/rook.obj");
		m_mesh[1] = ImportMesh("./resources/smooth_sphere.obj");
		m_mesh[2] = ImportMesh("./resources/one_plane.glb");
		m_mesh[3] = ImportMesh("./resources/sphere.obj");
		m_mesh[4] = ImportMesh("./resources/one_plane.glb");
		m_tex_base = Load_KTX("./resources/rook2/rook_base.ktx");
		m_tex_normal = Load_KTX("./resources/rook2/rook_normal.ktx");
		m_random.Init();

		glGenBuffers(1, &m_ubo);
		m_data = new Material_Uniform;
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (m_input_mode) {
			m_camera.OnUpdate(input, 1.0f, 0.1f, dt);
		}

		//Implement Camera Movement Functions
		if (input.Pressed(GLFW_KEY_LEFT_CONTROL)) {
			m_input_mode = !m_input_mode;
			input.SetRawMouseMode(window.GetHandle(), m_input_mode);
		}

		//Rook
		m_mesh_model[0] = glm::rotate(cos((float)m_time * 1.1f), glm::vec3(1.0, 1.0, 0.0)) * glm::scale(glm::vec3(2.0f));
		//Sphere
		m_mesh_model[1] = glm::translate(glm::vec3(-2.0f)) * glm::scale(glm::vec3(0.5f));
		//Plane
		m_mesh_model[2] = glm::translate(glm::vec3(-5.0f)) * glm::rotate(90.0f, glm::vec3(0.5f, 0.5f, 0.0)) * glm::scale(glm::vec3(2.0f));
		//Light
		m_mesh_model[3] = glm::translate(m_light_pos) * glm::scale(glm::vec3(0.5f));
		//Floor Plane
		m_mesh_model[4] = glm::translate(glm::vec3(0.0f, -3.0f, 0.0f)) * glm::scale(glm::vec3(3.0f));

		m_light_view = glm::lookAt(m_light_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		Material_Uniform temp_material = { m_light_pos, 0.0, m_diffuse_albedo, 0.0, m_specular_albedo, m_specular_power, m_ambient, 0.0 };
		memcpy(m_data, &temp_material, sizeof(Material_Uniform));
	}
	void OnDraw() {
		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		//glEnable(GL_CULL_FACE);

		//Shadow pass
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_FBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		//glCullFace(GL_FRONT);
		glUseProgram(m_shadow_program);
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_light_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_light_proj));
		DrawMesh(m_mesh[0], m_mesh_model[0]);
		DrawMesh(m_mesh[1], m_mesh_model[1]);
		DrawMesh(m_mesh[2], m_mesh_model[2]);
		DrawMesh(m_mesh[4], m_mesh_model[4]);
		//glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(m_phong_program);
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform3fv(3, 1, glm::value_ptr(m_camera.Eye()));
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(m_light_proj * m_light_view));


		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(Material_Uniform), m_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);

		/*glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_tex_shadow);*/
		glBindTextureUnit(0, m_tex_shadow);
		//glBindTextureUnit(1, m_tex_base);
		//glBindTextureUnit(1, m_tex_normal);
		glUniform1i(15, false);
		DrawMesh(m_mesh[0], m_mesh_model[0]);
		DrawMesh(m_mesh[1], m_mesh_model[1]);
		DrawMesh(m_mesh[2], m_mesh_model[2]);
		DrawMesh(m_mesh[4], m_mesh_model[4]);
		glUniform1i(15, true);
		DrawMesh(m_mesh[3], m_mesh_model[3]);


	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::DragFloat3("Light Position", glm::value_ptr(m_light_pos), 0.1f);
		ImGui::ColorEdit3("Diffuse Albedo", glm::value_ptr(m_diffuse_albedo));
		ImGui::ColorEdit3("Specular Albedo", glm::value_ptr(m_specular_albedo));
		ImGui::DragFloat("Specular Power", &m_specular_power, 0.1f);
		ImGui::ColorEdit3("Ambient", glm::value_ptr(m_ambient));

		ImGui::End();
	}
	GLuint CreateShadowFrameBuffer() {
		GLuint depthMapFBO;
		glGenFramebuffers(1, &depthMapFBO);

		GLuint depthMap;
		glGenTextures(1, &depthMap);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
			SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		m_tex_shadow = depthMap;
		return depthMapFBO;
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
#endif //SHADOW_PROJ