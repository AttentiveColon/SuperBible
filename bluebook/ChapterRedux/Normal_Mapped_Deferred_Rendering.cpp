#include "Defines.h"
#ifdef NORMAL_MAPPED_DEFERRED_RENDERING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

static const GLchar* deferred_input_vertex_shader_source = R"(
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
in vec2 uv;

layout (location = 0) 
uniform mat4 model;
layout (location = 1)
uniform mat4 view;
layout (location = 2)
uniform mat4 proj;

layout (location = 15)
uniform int material_id;

layout (binding = 5) uniform MODEL_MATRIX_BUFFER
{
	mat4 model_matrix[1000];
};


out VS_OUT
{
	vec3 ws_coords;
	vec3 N;
	vec2 uv;
	flat uint material_id;
} vs_out;

void main()
{	
	
	vec4 P = model * model_matrix[gl_InstanceID] * vec4(position, 1.0);

	vs_out.ws_coords = P.xyz; 
	vs_out.N = mat3(model) * normal;
	vs_out.uv = uv;
	vs_out.material_id = uint(material_id);

	gl_Position = proj * view * P;
}
)";

static const GLchar* deferred_input_fragment_shader_source = R"(
#version 450 core

layout (location = 0) out uvec4 color0;
layout (location = 1) out vec4 color1;

in VS_OUT
{
	vec3 ws_coords;
	vec3 N;
	vec2 uv;
	flat uint material_id;
} fs_in;

layout (binding = 0) uniform sampler2D u_diffuse_texture;

layout (location = 5)
uniform vec3 light_color = vec3(1.0);

void main()
{
	uvec4 outvec0 = uvec4(0);
	vec4 outvec1 = vec4(0);

	vec3 color = texture(u_diffuse_texture, fs_in.uv).rgb * light_color;

	outvec0.x = packHalf2x16(color.xy);
	outvec0.y = packHalf2x16(vec2(color.z, fs_in.N.x));
	outvec0.z = packHalf2x16(fs_in.N.yz);
	outvec0.w = fs_in.material_id;

	outvec1.xyz = fs_in.ws_coords;
	outvec1.w = 40.0;

	color0 = outvec0;
	color1 = outvec1;
}
)";

static ShaderText deferred_input_shader_text[] = {
	{GL_VERTEX_SHADER, deferred_input_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, deferred_input_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* deferred_lighting_vertex_shader_source = R"(
#version 450 core

void main(void)
{
    const vec4 verts[4] = vec4[4](vec4(-1.0, -1.0, 0.5, 1.0),
                                  vec4( 1.0, -1.0, 0.5, 1.0),
                                  vec4(-1.0,  1.0, 0.5, 1.0),
                                  vec4( 1.0,  1.0, 0.5, 1.0));

    gl_Position = verts[gl_VertexID];
}
)";

static const GLchar* deferred_lighting_fragment_shader_source = R"(
#version 450 core

layout (location = 0) out vec4 color_out;

layout (binding = 0) uniform usampler2D gbuf_tex0;
layout (binding = 1) uniform sampler2D gbuf_tex1;


layout (location = 11)
uniform vec3 cam_pos;
layout (location = 12)
uniform int light_num;

struct LightData
{
	vec3 light_pos;
	float light_intensity;
	vec3 light_color;
	float pad0;
};

layout (binding = 0) uniform LightUniform
{
	LightData light_data[4];
};

struct fragment_into_t
{
	vec3 color;
	vec3 normal;
	float specular_power;
	vec3 ws_coord;
	uint material_id;
};

vec3 CalculateAttenuation(float distance, float light_intensity, vec3 light_color)
{
	float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
	attenuation *= light_intensity;
	return light_color * attenuation;
}

void unpackGBuffer(ivec2 coord, out fragment_into_t fragment)
{
	uvec4 data0 = texelFetch(gbuf_tex0, ivec2(coord), 0);
	vec4 data1 = texelFetch(gbuf_tex1, ivec2(coord), 0);
	vec2 temp;

	temp = unpackHalf2x16(data0.y);
	fragment.color = vec3(unpackHalf2x16(data0.x), temp.x);
	fragment.normal = normalize(vec3(temp.y, unpackHalf2x16(data0.z)));
	fragment.material_id = data0.w;
	
	fragment.ws_coord = data1.xyz;
	fragment.specular_power = data1.w;
}

vec4 light_fragment(fragment_into_t fragment)
{
	vec3 diffuse_albedo = fragment.color;	

	vec3 diffuse = diffuse_albedo;
	vec3 specular = vec3(0.0);
	vec3 ambient = vec3(0.0);

	if (fragment.material_id != 0)
	{
		diffuse = vec3(0.0);
		for (int i = 0; i < light_num; ++i)
		{
			float distance = length(light_data[i].light_pos - fragment.ws_coord);
			vec3 attenuation = CalculateAttenuation(distance, light_data[i].light_intensity, light_data[i].light_color);
			
			vec3 N = normalize(fragment.normal);
			vec3 L = normalize(light_data[i].light_pos - fragment.ws_coord);
			vec3 V = normalize(cam_pos - fragment.ws_coord);
			vec3 R = reflect(-L, N);

			diffuse += max(dot(N, L), 0.0) * diffuse_albedo * attenuation;
			specular += pow(max(dot(V, R), 0.0), fragment.specular_power) * attenuation;
		}
		ambient += diffuse_albedo * vec3(0.05);
	}

	return vec4(ambient + diffuse + specular, 1.0);	
}

void main()
{
	fragment_into_t fragment;
	unpackGBuffer(ivec2(gl_FragCoord.xy), fragment);
	color_out = light_fragment(fragment);
}
)";

static ShaderText deferred_lighting_shader_text[] = {
	{GL_VERTEX_SHADER, deferred_lighting_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, deferred_lighting_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct LightData {
	glm::vec3 light_pos;
	float light_intensity;
	glm::vec3 light_color;
	float pad0;
};

struct Mesh {
	GLuint vao;
	size_t count;
};

static void DrawMesh(const Mesh& mesh) {
	glBindVertexArray(mesh.vao);
	glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, (void*)0);
}

static void DrawMesh(const Mesh& mesh, size_t count) {
	glBindVertexArray(mesh.vao);
	glDrawElementsInstanced(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, (void*)0, count);
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


struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	ObjMesh m_cube[3];
	GLuint m_tex, m_white_tex;

	//glm::vec3 m_view_pos = glm::vec3(-10.0f);

	//glm::vec3 m_light_pos = glm::vec3(1.0f, 41.0f, 50.0f);

	SB::Camera m_camera;
	bool m_input_mode = false;

	//Meshes
	Mesh m_mesh;
	GLuint m_mesh_diffuse_tex, m_mesh_normal_tex;

	//Model position data
	GLuint m_model_matrix_buffer;
	GLuint m_light_model_matrix_buffer;
	
	//Geometry buffer data
	GLuint m_vao;
	GLuint m_deferred_input_program, m_deferred_lighting_program;
	GLuint m_gbuffer;
	GLuint m_gbuffer_textures[3];

	//Light Data
	LightData m_light_data[4];
	GLuint m_light_ubo;
	vector<LightData> m_lights;
	vector<glm::vec3> m_lights_to;
	vector<glm::vec3> m_lights_from;

	//Random Engine
	Random m_random;

#define OBJ_ARRAY_SIZE 10
#define OBJ_SCALING 0.1f

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_deferred_input_program = LoadShaders(deferred_input_shader_text);
		m_deferred_lighting_program = LoadShaders(deferred_lighting_shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 8.0f, 15.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_mesh = ImportMesh("./resources/rook2/rook.obj");
		m_mesh_diffuse_tex = Load_KTX("./resources/rook2/rook_base.ktx");
		m_mesh_normal_tex = Load_KTX("./resources/rook2/rook_normal.ktx");
		m_model_matrix_buffer = CreateInstancePositions();
		m_light_model_matrix_buffer = CreateLightInstancePositionsBuffer();

		m_random.Init();

		glEnable(GL_DEPTH_TEST);

		CreateGBuffer();
		CreateWhiteTex();
		AddLight(30.0f, 5.0f);
		CreateUniformLightBuffer();
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

		if (m_input_mode) {
			m_camera.OnUpdate(input, 3.0f, 0.2f, dt);
		}

		//Implement Camera Movement Functions
		if (input.Pressed(GLFW_KEY_LEFT_CONTROL)) {
			m_input_mode = !m_input_mode;
			input.SetRawMouseMode(window.GetHandle(), m_input_mode);
		}

		UpdateLights();
		UpdateLightUniform();
		UpdateLightInstanceBuffer();

		if (input.Pressed(GLFW_KEY_SPACE)) {
			AddLight(m_random.Float() * 60.0f, m_random.Float() * 30.0f);
		}
	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DeferredRender();

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Text("Light Count: %d", m_lights.size());
		ImGui::End();
	}
	bool RandomBool() {
		return m_random.Float() > 0.5f;
	}
	void AddLight(float scale, float offset) {
		float bias = -(OBJ_ARRAY_SIZE * OBJ_SCALING / 2.0f);
		glm::vec3 position = glm::vec3((m_random.Float() * 2.0 - 1.0) * bias, (m_random.Float() * 2.0 - 1.0) * bias, (m_random.Float() * 2.0 - 1.0) * bias);
		float intensity = m_random.Float() * 2.0f;
		glm::vec3 light_color = glm::vec3(m_random.Float(), m_random.Float(), m_random.Float());
		float pad = 0.0f;
		LightData ld = { position, intensity, light_color, pad };

		m_lights.push_back(ld);

		glm::vec3 light_to = glm::vec3((m_random.Float() * 2.0 - 1.0) * bias, (m_random.Float() * 2.0 - 1.0) * bias, (m_random.Float() * 2.0 - 1.0) * bias);
		m_lights_to.push_back(light_to);
		m_lights_from.push_back(ld.light_pos);
	}
	void UpdateLights() {
		for (int i = 0; i < m_lights.size(); ++i) {
			m_lights[i].light_pos = glm::mix(m_lights_from[i], m_lights_to[i], sin(m_time * 0.1));
		}
	}
	void CreateUniformLightBuffer() {
		glGenBuffers(1, &m_light_ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, m_light_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(LightData) * m_lights.size(), nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	void UpdateLightUniform() {
		glBindBuffer(GL_UNIFORM_BUFFER, m_light_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(LightData) * m_lights.size(), nullptr, GL_DYNAMIC_DRAW);

		LightData* data = (LightData*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(LightData) * m_lights.size(), GL_MAP_WRITE_BIT);
		for (int i = 0; i < m_lights.size(); ++i) {
			data[i].light_pos = m_lights[i].light_pos;
			data[i].light_intensity = m_lights[i].light_intensity;
			data[i].light_color = m_lights[i].light_color;
			data[i].pad0 = 0.0;
		}

		glUnmapBuffer(GL_UNIFORM_BUFFER);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	void CreateWhiteTex() {
		static const GLubyte white_texture[] = { 0xff, 0xff, 0xff, 0xff };

		glGenTextures(1, &m_white_tex);
		glBindTexture(GL_TEXTURE_2D, m_white_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_texture);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void CreateGBuffer() {
		glGenFramebuffers(1, &m_gbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer);

		glGenTextures(3, m_gbuffer_textures);
		glBindTexture(GL_TEXTURE_2D, m_gbuffer_textures[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, 1600, 900);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, m_gbuffer_textures[1]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1600, 900);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, m_gbuffer_textures[2]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 1600, 900);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_gbuffer_textures[0], 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_gbuffer_textures[1], 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_gbuffer_textures[2], 0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);

	}
	GLuint CreateInstancePositions() {
		float scaling = OBJ_SCALING;
		int size = OBJ_ARRAY_SIZE;
		vector<glm::mat4> model_array;
		for (int z = 0; z < size; ++z)
			for (int y = 0; y < size; ++y)
				for (int x = 0; x < size; ++x) {
					float bias = -((size * scaling) / 2.0);
					model_array.push_back(glm::translate(glm::vec3(x * scaling + bias, y * scaling + bias, z * scaling + bias)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)));
				}

		GLuint model_matrix_buffer;
		glCreateBuffers(1, &model_matrix_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, model_matrix_buffer);
		glBufferStorage(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * pow(OBJ_ARRAY_SIZE, 3), &model_array[0], 0);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		return model_matrix_buffer;
	}
	GLuint CreateLightInstancePositionsBuffer() {
		GLuint light_model_matrix_buffer;
		glCreateBuffers(1, &light_model_matrix_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, light_model_matrix_buffer);
		glBufferStorage(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 100, nullptr, GL_MAP_WRITE_BIT);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
		return light_model_matrix_buffer;
	}
	void UpdateLightInstanceBuffer() {
		vector<glm::mat4> light_model_data;
		for (int i = 0; i < m_lights.size(); ++i) {
			light_model_data.push_back(glm::translate(m_lights[i].light_pos) * glm::scale(glm::vec3(1.0f)));
		}

		glBindBuffer(GL_UNIFORM_BUFFER, m_light_model_matrix_buffer);
		glm::mat4* data = (glm::mat4*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4) * light_model_data.size(), GL_MAP_WRITE_BIT);
		for (int i = 0; i < light_model_data.size(); ++i) {
			data[i] = light_model_data[i];
		}
		glUnmapNamedBuffer(m_light_model_matrix_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	void DeferredRender() {
		float time = float(m_time) * 0.5f;
		static const GLuint uint_zeros[] = { 0, 0, 0, 0 };
		static const GLfloat float_zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		static const GLfloat float_ones[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

		glm::vec3 light_pos = glm::vec3(sin(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f);
		glm::mat4 light_model = glm::translate(light_pos) * glm::scale(glm::vec3(1.0));

		glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer);
		glDrawBuffers(2, draw_buffers);
		glClearBufferuiv(GL_COLOR, 0, uint_zeros);
		glClearBufferuiv(GL_COLOR, 1, uint_zeros);
		glClearBufferfv(GL_DEPTH, 0, float_ones);

		//Render Scene without lighting
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glUseProgram(m_deferred_input_program);
		glBindTextureUnit(0, m_mesh_diffuse_tex);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform3fv(5, 1, glm::value_ptr(glm::vec3(1.0f)));
		glUniform1i(15, 1);
		glBindBufferBase(GL_UNIFORM_BUFFER, 5, m_model_matrix_buffer);
		DrawMesh(m_mesh, pow(OBJ_ARRAY_SIZE, 3));

		//Render light spheres
		for (int i = 0; i < m_lights.size(); ++i) {
			glm::mat4 light_model = glm::translate(m_lights[i].light_pos) * glm::scale(glm::vec3(1.0));
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(light_model));
			glBindTextureUnit(0, m_white_tex);
			glUniform3fv(5, 1, glm::value_ptr(m_lights[i].light_color));
			glUniform1i(15, 0);
			DrawMesh(m_mesh);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glDrawBuffer(GL_BACK);
		glDisable(GL_DEPTH_TEST);
		glUseProgram(m_deferred_lighting_program);
		glBindVertexArray(m_vao);
		glBindTextureUnit(0, m_gbuffer_textures[0]);
		glBindTextureUnit(1, m_gbuffer_textures[1]);
		glUniform3fv(11, 1, glm::value_ptr(m_camera.m_cam_position));
		glUniform1i(12, m_lights.size());
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_light_ubo);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

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
#endif //NORMAL_MAPPED_DEFERRED_RENDERING