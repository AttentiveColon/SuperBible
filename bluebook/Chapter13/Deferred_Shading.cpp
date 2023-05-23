#include "Defines.h"
#ifdef DEFERRED_SHADING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* deferred_input_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 0) 
uniform mat4 model;
layout (location = 1)
uniform mat4 view;
layout (location = 2)
uniform mat4 proj;

layout (location = 15)
uniform int material_id;


out VS_OUT
{
	vec3 ws_coords;
	vec3 N;
	vec2 uv;
	flat uint material_id;
} vs_out;

void main()
{	
	
	vec4 P = model * vec4(position, 1.0);

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

layout (location = 0)
uniform vec3 light_pos = vec3(0.0);
uniform vec3 light_color = vec3(1.0);

layout (location = 11)
uniform vec3 cam_pos;

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
		for (int i = 0; i < 4; ++i)
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

static const GLchar* phong_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;
layout (location = 3)
in vec3 tangent;

layout (location = 0) 
uniform mat4 model;
layout (location = 1)
uniform mat4 view;
layout (location = 2)
uniform mat4 proj;

layout (location = 10)
uniform vec3 light_position;
layout (location = 11)
uniform vec3 cam_pos;


out VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
} vs_out;

void main()
{	
	vec4 P = model * vec4(position, 1.0);
	vs_out.N = mat3(model) * normal;
	vs_out.L = light_position - P.xyz;
	vs_out.V = cam_pos - P.xyz;
	vs_out.uv = uv;

	gl_Position = proj * view * P;
}

)";


static const GLchar* phong_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D diffuse_texture;

layout (location = 12)
uniform vec3 ambient_level = vec3(0.05);
layout (location = 13)
uniform vec3 specular_albedo = vec3(1.0);
layout (location = 14)
uniform float specular_power = 40.0;
layout (location = 15)
uniform bool is_lit = true;

layout (location = 16)
uniform vec3 light_color = vec3(1.0);
layout (location = 17)
uniform float light_intensity = 5.0;

out vec4 color;

in VS_OUT
{
	vec3 N;
	vec3 L;
	vec3 V;
	vec2 uv;
} fs_in;

vec3 CalculateAttenuation(float distance, float light_intensity, vec3 light_color)
{
	float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
	attenuation *= light_intensity;
	return light_color * attenuation;
}

void main()
{
	vec3 diffuse_albedo = texture(diffuse_texture, fs_in.uv).rgb;

	vec3 diffuse = diffuse_albedo;
	vec3 specular = vec3(0.0);
	vec3 ambient = vec3(0.0);

	if (is_lit)
	{
		diffuse = vec3(0.0);
		vec3 attenuation = CalculateAttenuation(length(fs_in.L), light_intensity, light_color);

		vec3 N = normalize(fs_in.N);
		vec3 L = normalize(fs_in.L);
		vec3 V = normalize(fs_in.V);
		vec3 R = reflect(-L, N);
		
		diffuse = max(dot(N, L), 0.0) * diffuse_albedo * attenuation;
		specular = pow(max(dot(V, R), 0.0), specular_power) * specular_albedo * attenuation;
		ambient = diffuse_albedo * ambient_level;

	}

	color = vec4(diffuse + ambient + specular, 1.0);
}
)";

static ShaderText phong_shader_text[] = {
	{GL_VERTEX_SHADER, phong_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, phong_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct LightData {
	glm::vec3 light_pos;
	float light_intensity;
	glm::vec3 light_color;
	float pad0;
};


struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_phong_program, m_light_program;
	ObjMesh m_cube[3];
	GLuint m_tex, m_white_tex;

	glm::vec3 m_view_pos = glm::vec3(-10.0f);

	glm::vec3 m_light_pos = glm::vec3(1.0f, 41.0f, 50.0f);

	SB::Camera m_camera;
	bool m_input_mode = false;

	bool m_deferred_render = false;

	bool m_light_paused = false;
	float m_light_movement = 0.0;

	//Geometry buffer data
	GLuint m_vao;
	GLuint m_deferred_input_program, m_deferred_lighting_program;
	GLuint m_gbuffer;
	GLuint m_gbuffer_textures[3];

	//Light Data
	LightData m_light_data[4];
	GLuint m_light_ubo;

#define OBJ_ARRAY_SIZE 30
#define OBJ_SCALING 3.0f

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_phong_program = LoadShaders(phong_shader_text);
		m_deferred_input_program = LoadShaders(deferred_input_shader_text);
		m_deferred_lighting_program = LoadShaders(deferred_lighting_shader_text);
		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 8.0f, 15.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 1000.0);
		m_tex = Load_KTX("./resources/fiona.ktx");
		m_cube[0].Load_OBJ("./resources/smooth_sphere.obj");
		m_cube[1].Load_OBJ("./resources/cube.obj");
		m_cube[2].Load_OBJ("./resources/monkey.obj");

		glEnable(GL_DEPTH_TEST);

		CreateGBuffer();
		CreateWhiteTex();
		CreateLightData();
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

		UpdateLightData();
		UpdateLightUniform();
	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (!m_deferred_render)
			Render();
		else
			DeferredRender();

	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Deferred Render On", &m_deferred_render);
		ImGui::Checkbox("Light Paused", &m_light_paused);
		ImGui::DragFloat("Light Movement", &m_light_movement, 0.01f);
		ImGui::End();
	}
	void CreateUniformLightBuffer() {
		glGenBuffers(1, &m_light_ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, m_light_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(LightData) * 4, nullptr, GL_STATIC_DRAW);

		LightData* data = (LightData*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(LightData) * 4, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		for (int i = 0; i < 4; ++i) {
			data[i].light_pos = m_light_data[i].light_pos;
			data[i].light_color = m_light_data[i].light_color;
			data[i].light_intensity = m_light_data[i].light_intensity;
		}

		glUnmapBuffer(GL_UNIFORM_BUFFER);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	void UpdateLightUniform() {
		glBindBuffer(GL_UNIFORM_BUFFER, m_light_ubo);

		LightData* data = (LightData*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(LightData) * 4, GL_MAP_WRITE_BIT);
		for (int i = 0; i < 4; ++i) {
			data[i].light_pos = m_light_data[i].light_pos;
			data[i].light_intensity = m_light_data[i].light_intensity;
			data[i].light_color = m_light_data[i].light_color;
			data[i].pad0 = 0.0;
		}

		glUnmapBuffer(GL_UNIFORM_BUFFER);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	void CreateLightData() {
		float time = float(m_time * 0.5);
		m_light_data[0] = { glm::vec3(sin(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f), 0.5f, glm::vec3(1.0f, 0.0, 0.0), 0.0f};
		time += 0.25;
		m_light_data[1] = { glm::vec3(cos(time) * 30.0f + 5.0f, sin(time) * 30.0f + 5.0f, sin(time) * 30.0f + 5.0f), 0.5f, glm::vec3(0.0f, 1.0, 0.0), 0.0f};
		time += 0.50;
		m_light_data[2] = { glm::vec3(sin(time) * 30.0f + 5.0f, sin(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f), 0.5f, glm::vec3(0.0f, 0.0, 1.0), 0.0f};
		time += 0.75;
		m_light_data[3] = { glm::vec3(cos(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f, sin(time) * 30.0f + 5.0f), 0.5f, glm::vec3(1.0f, 1.0, 1.0), 0.0f};
	}
	void UpdateLightData() {
		float time = float(m_time * 0.5);
		m_light_data[0] = { glm::vec3(sin(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f), 7.5f, glm::vec3(1.0f, 0.0, 0.0), 0.0f};
		time += 0.25;
		m_light_data[1] = { glm::vec3(cos(time) * 30.0f + 5.0f, sin(time) * 30.0f + 5.0f, sin(time) * 30.0f + 5.0f), 5.5f, glm::vec3(0.0f, 1.0, 0.0), 0.0f};
		time += 0.50;
		m_light_data[2] = { glm::vec3(sin(time) * 30.0f + 5.0f, sin(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f), 4.5f, glm::vec3(0.0f, 0.0, 1.0), 0.0f};
		time += 0.75;
		m_light_data[3] = { glm::vec3(cos(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f, sin(time) * 30.0f + 5.0f), 3.5f, glm::vec3(1.0f, 1.0, 1.0),  0.0f};
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
	void DeferredRender() {
		float time = float(m_time) * 0.5f;
		static const GLuint uint_zeros[] = { 0, 0, 0, 0 };
		static const GLfloat float_zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		static const GLfloat float_ones[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

		glm::vec3 light_pos = glm::vec3(sin(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f);
		glm::mat4 light_model = glm::translate(light_pos) * glm::scale(glm::vec3(3.0));

		glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer);
		glDrawBuffers(2, draw_buffers);
		glClearBufferuiv(GL_COLOR, 0, uint_zeros);
		glClearBufferuiv(GL_COLOR, 1, uint_zeros);
		glClearBufferfv(GL_DEPTH, 0, float_ones);
		//Render Scene without lighting
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glUseProgram(m_deferred_input_program);
		glBindTextureUnit(0, m_tex);
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform3fv(5, 1, glm::value_ptr(glm::vec3(1.0f)));
		glUniform1i(15, 1);
		float scaling = OBJ_SCALING;
		int size = OBJ_ARRAY_SIZE;
		glm::mat4 model = glm::mat4(1.0f);
		for (int z = 0; z < size; ++z)
			for (int y = 0; y < size; ++y)
				for (int x = 0; x < size; ++x) {
					model = glm::translate(glm::vec3(x * scaling, y * scaling, z * scaling)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
					glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
					m_cube[0].OnDraw();
				}
		//Render light spheres
		for (int i = 0; i < 4; ++i) {
			glm::mat4 light_model = glm::translate(m_light_data[i].light_pos) * glm::scale(glm::vec3(3.0));
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(light_model));
			glBindTextureUnit(0, m_white_tex);
			glUniform3fv(5, 1, glm::value_ptr(m_light_data[i].light_color));
			glUniform1i(15, 0);
			m_cube[0].OnDraw();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glDrawBuffer(GL_BACK);
		glDisable(GL_DEPTH_TEST);
		glUseProgram(m_deferred_lighting_program);
		glBindVertexArray(m_vao);
		glBindTextureUnit(0, m_gbuffer_textures[0]);
		glBindTextureUnit(1, m_gbuffer_textures[1]);
		glUniform3fv(0, 1, glm::value_ptr(light_pos));
		glUniform3fv(11, 1, glm::value_ptr(m_camera.m_cam_position));
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_light_ubo);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	}
	void Render() {
		float time = float(m_time) * 0.5f;
		if (m_light_paused) { time = m_light_movement; }
		

		glm::vec3 light_pos = glm::vec3(sin(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f, cos(time) * 30.0f + 5.0f);
		glm::mat4 light_model = glm::translate(light_pos) * glm::scale(glm::vec3(3.0));

		glEnable(GL_DEPTH_TEST);
		glUseProgram(m_phong_program);
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform3fv(10, 1, glm::value_ptr(light_pos));
		glUniform3fv(11, 1, glm::value_ptr(m_camera.m_cam_position));

		glBindTextureUnit(0, m_white_tex);
		glUniform1i(15, 0);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(light_model));
		m_cube[0].OnDraw();

		glBindTextureUnit(0, m_tex);
		glUniform1i(15, 1);
		float scaling = OBJ_SCALING;
		int size = OBJ_ARRAY_SIZE;
		glm::mat4 model = glm::mat4(1.0f);
		for (int z = 0; z < size; ++z) 
			for (int y = 0; y < size; ++y)
				for (int x = 0; x < size; ++x) {
					model = glm::translate(glm::vec3(x * scaling, y * scaling, z * scaling)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
					glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
					m_cube[0].OnDraw();
				}	
		glDisable(GL_DEPTH_TEST);
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
#endif //DEFERRED_SHADING