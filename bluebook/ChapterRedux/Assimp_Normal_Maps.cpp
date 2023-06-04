#include "Defines.h"
#ifdef ASSIMP_NORMAL_MAPS
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <omp.h>


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

layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient;
	float pad2;
	vec3 rim_color;
	float rim_power;
};

out VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
} vs_out;

void main(void)
{

    vec4 P = u_model * vec4(position, 1.0);
	vs_out.N = mat3(u_model) * normal;
	vs_out.L = light_pos - P.xyz;
	vs_out.V = u_cam_pos - P.xyz;
	vs_out.uv = texcoord;

	gl_Position = u_proj * u_view * P;
}
)";

static const GLchar* blinn_phong_fragment_shader_source = R"(
#version 450 core

layout  (binding = 0)
uniform sampler2D u_diffuse;

layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient;
	float pad2;
	vec3 rim_color;
	float rim_power;
};

in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
} fs_in;

out vec4 color;


void main()
{
	vec3 N = normalize(fs_in.N);
	vec3 L = normalize(fs_in.L);
	vec3 V = normalize(fs_in.V);

	vec3 H = normalize(L + V);

	vec3 diffuse = max(dot(N, L), 0.0) * texture(u_diffuse, fs_in.uv).rgb;
	vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;

	color = vec4(ambient + diffuse + specular, 1.0);
}
)";

static ShaderText blinn_phong_shader_text[] = {
	{GL_VERTEX_SHADER, blinn_phong_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, blinn_phong_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* normal_vertex_shader_source = R"(
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

layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient;
	float pad2;
	vec3 rim_color;
	float rim_power;
};

out VS_OUT
{
	vec2 uv;
	vec3 eye_dir;
	vec3 light_dir;
} vs_out;

void main(void)
{
	mat3 n_matrix = transpose(inverse(mat3(u_model)));
    vec4 P = u_model * vec4(position, 1.0);

	vec3 N = normalize(n_matrix * normal);
	vec3 T = normalize(n_matrix * tangent);
	vec3 B = normalize(n_matrix * bitangent);

	//mat3 TBN = transpose(mat3(T, B, N));

	vec3 L = light_pos - P.xyz;
	//vs_out.light_dir = normalize(TBN * L);
	vs_out.light_dir = normalize(vec3(dot(L, T), dot(L, B), dot(L, N)));

	vec3 V = u_cam_pos - P.xyz;
	vs_out.eye_dir = normalize(vec3(dot(V, T), dot(V, B), dot(V, N)));

	vs_out.uv = texcoord;
	gl_Position = u_proj * u_view * P;
}
)";

static const GLchar* normal_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D u_diffuse;

layout (binding = 1)
uniform sampler2D u_normal_map;

layout (binding = 0, std140)
uniform Material
{
	vec3 light_pos;
	float pad0;
	vec3 diffuse_albedo;
	float pad1;
	vec3 specular_albedo;
	float specular_power;
	vec3 ambient;
	float pad2;
	vec3 rim_color;
	float rim_power;
};

in VS_OUT
{
	vec2 uv;
	vec3 eye_dir;
	vec3 light_dir;
} fs_in;

out vec4 color;

void main()
{
	vec3 V = normalize(fs_in.eye_dir);
	vec3 L = normalize(fs_in.light_dir);
	vec3 N = normalize(texture(u_normal_map, fs_in.uv).rgb * 2.0 - vec3(1.0));


	vec3 R = reflect(-L, N);

	vec3 diffuse_color = texture(u_diffuse, fs_in.uv).rgb;
	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_color;
	vec3 specular = max(pow(dot(R, V), specular_power), 0.0) * specular_albedo;

	
	color = vec4(diffuse + specular, 1.0);
	//color = vec4(fs_in.uv.x, fs_in.uv.y, 0.0, 1.0);
}
)";



static ShaderText normal_shader_text[] = {
	{GL_VERTEX_SHADER, normal_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, normal_fragment_shader_source, NULL},
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
	glm::vec3 rim_color;
	float rim_power;
};



struct Meshy
{
	GLuint vao;
	size_t count;
};

Meshy ImportMesh(const char* filename) {
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

	/*for (int i = 0; i < mesh->mNumFaces * 3; ++i) {
		std::cout << index_data[i] << std::endl;
	}*/

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

	Meshy result;
	result.vao = vao;
	result.count = index_data.size();
	return result;
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_normal_program;
	GLuint m_phong_program;

	bool m_normal_program_active = true;

	GLuint m_ubo;
	Material_Uniform* m_data;

	glm::vec3 m_light_pos = glm::vec3(100.0f, 100.0f, 100.0f);
	glm::vec3 m_diffuse_albedo = glm::vec3(0.5f, 0.2f, 0.7f);
	glm::vec3 m_specular_albedo = glm::vec3(0.7f);
	float m_specular_power = 128.0f;
	glm::vec3 m_ambient = vec3(0.1f, 0.1f, 0.1f);
	glm::vec3 m_rim_color = vec3(0.6f);
	float m_rim_power = 128.0f;

	ObjMesh m_cube;


	GLuint m_tex_base, m_tex_normal;
	glm::mat4 m_mesh_rotation;

	SB::Camera m_camera;
	bool m_input_mode = false;

	Random m_random;

	Meshy m_mesh;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		int num_threads = omp_get_max_threads();
		m_normal_program = LoadShaders(normal_shader_text);
		m_phong_program = LoadShaders(blinn_phong_shader_text);


		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 0.1f, 0.3f), glm::vec3(0.0f, 0.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);

		m_mesh = ImportMesh("./resources/rook2/rook.obj");
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

		m_mesh_rotation = glm::rotate(cos((float)m_time * 0.1f), glm::vec3(0.0, 1.0, 0.0));


		Material_Uniform temp_material = { m_light_pos, 0.0, m_diffuse_albedo, 0.0, m_specular_albedo, m_specular_power, m_ambient, 0.0, m_rim_color, m_rim_power };
		memcpy(m_data, &temp_material, sizeof(Material_Uniform));
	}
	void OnDraw() {
		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glEnable(GL_DEPTH_TEST);

		if (m_normal_program_active)
			glUseProgram(m_normal_program);
		else
			glUseProgram(m_phong_program);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_mesh_rotation));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform3fv(3, 1, glm::value_ptr(m_camera.Eye()));


		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(Material_Uniform), m_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);


		glBindTextureUnit(0, m_tex_base);
		glBindTextureUnit(1, m_tex_normal);
		glBindVertexArray(m_mesh.vao);
		glDrawElements(GL_TRIANGLES, m_mesh.count, GL_UNSIGNED_INT, (void*)0);

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
		ImGui::ColorEdit3("Rim Color", glm::value_ptr(m_rim_color));
		ImGui::DragFloat("Rim Power", &m_rim_power, 0.1f);
		ImGui::Checkbox("Normal Map Active", &m_normal_program_active);
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
#endif //ASSIMP_NORMAL_MAPS