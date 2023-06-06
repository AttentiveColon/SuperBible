#include "Defines.h"
#ifdef CUBE_MAP_REDUX
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"

static const GLchar* skybox_vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;

out VS_OUT
{
    vec3    tc;
} vs_out;

layout (location = 0)
uniform vec3 u_eye;
layout (location = 1)
uniform mat4 u_view;
layout (location = 2)
uniform mat4 u_proj;
layout (location = 3)
uniform vec3 u_scale;

void main(void)
{

    vs_out.tc = position;

    gl_Position = u_proj * u_view * vec4((position * u_scale) + u_eye, 1.0);
}

)";

static const GLchar* skybox_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform samplerCube tex_cubemap;

in VS_OUT
{
    vec3    tc;
} fs_in;

layout (location = 0) out vec4 color;

void main(void)
{
    color = texture(tex_cubemap, fs_in.tc);
}
)";

static ShaderText skybox_shader_text[] = {
	{GL_VERTEX_SHADER, skybox_vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, skybox_fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLchar* normal_vertex_shader_source = R"(
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
uniform mat4 u_model;
layout (location = 1)
uniform mat4 u_view;
layout (location = 2)
uniform mat4 u_proj;

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
	vec3 normal;
	vec3 view;
} vs_out;

void main(void)
{
	mat4 mv_matrix = u_view * u_model;
	mat3 n_matrix = transpose(inverse(mat3(mv_matrix)));

    vec4 P = mv_matrix * vec4(position, 1.0);
	vec3 V = P.xyz;
	vec3 N = normalize(n_matrix * normal);
	vec3 T = normalize(n_matrix * tangent.xyz);
	vec3 B = cross(N, tangent);

	vec3 L = light_pos - P.xyz;
	vs_out.light_dir = normalize(vec3(dot(L, T), dot(L, B), dot(L, N)));

	V = -P.xyz;
	vs_out.eye_dir = normalize(vec3(dot(V, T), dot(V, B), dot(V, N)));

	vs_out.uv = uv;
	vs_out.normal = mat3(mv_matrix) * normal;
	vs_out.view = P.xyz;
	gl_Position = u_proj * P;
}
)";

static const GLchar* normal_fragment_shader_source = R"(
#version 450 core

layout (binding = 0)
uniform sampler2D u_diffuse;

layout (binding = 1)
uniform sampler2D u_normal_map;

layout (binding = 2)
uniform sampler2D u_env_map;

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
	vec3 normal;
	vec3 view;
} fs_in;

out vec4 color;

vec3 calculate_rim(vec3 N, vec3 V)
{
	float f = 1.0 - dot(N, V);
	f = smoothstep(0.0, 1.0, f);
	f = pow(f, rim_power);
	return f * rim_color;
}

void main()
{
	vec2 normal_uv = vec2(fs_in.uv.x, -fs_in.uv.y);
	vec2 diffuse_uv = vec2(fs_in.uv.x, -fs_in.uv.y);

	vec3 V = normalize(fs_in.eye_dir);
	vec3 L = normalize(fs_in.light_dir);
	vec3 N = normalize(texture(u_normal_map, normal_uv).rgb * 2.0 - 1.0);

	//vec3 N = normalize(fs_in.normal);
	vec3 R = reflect(-L, N);

	vec3 diffuse_color = texture(u_diffuse, diffuse_uv).rgb;
	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_color;
	vec3 specular = max(pow(dot(R, V), specular_power), 0.0) * specular_albedo;

	vec3 rim = calculate_rim(fs_in.normal, fs_in.view);

	vec3 u = normalize(fs_in.view);
	vec3 r = reflect(u, normalize(fs_in.normal));
	vec2 tc;
	tc.y = r.y;
	r.y = 0.0;
	tc.x = normalize(r).x * 0.5;
	float s = sign(r.z) * 0.5;
	tc.s = 0.75 - s * (0.5 - tc.s);
	tc.t = 0.5 + 0.5 * tc.t;
	
	vec3 enviroment = texture(u_env_map, tc).rgb;	

	color = vec4(enviroment + diffuse, 1.0);
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

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_normal_program, m_skybox_program;

	GLuint m_skybox_vao;

	GLuint m_ubo;
	Material_Uniform* m_data;

	glm::vec3 m_light_pos = glm::vec3(100.0f, 100.0f, 100.0f);
	glm::vec3 m_diffuse_albedo = glm::vec3(0.5f, 0.2f, 0.7f);
	glm::vec3 m_specular_albedo = glm::vec3(0.7f);
	float m_specular_power = 128.0f;
	glm::vec3 m_ambient = vec3(0.1f, 0.1f, 0.1f);
	glm::vec3 m_rim_color = vec3(0.0f);
	float m_rim_power = 128.0f;

	ObjMesh m_cube;

	GLuint m_env_map;
	GLuint m_cube_map;

	GLuint m_tex_base, m_tex_normal;
	glm::mat4 m_mesh_rotation;

	SB::Camera m_camera;
	bool m_input_mode = false;

	Random m_random;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		m_normal_program = LoadShaders(normal_shader_text);
		m_skybox_program = LoadShaders(skybox_shader_text);

		m_camera = SB::Camera("Camera", glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 2.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.9, 0.01, 2000.0);

		//m_cube.Load_OBJ("./resources/rook2/rook.obj");
		//m_tex_base = Load_KTX("./resources/rook2/rook_base.ktx");
		//m_tex_normal = Load_KTX("./resources/rook2/rook_normal.ktx");
		m_cube.Load_OBJ("./resources/cube.obj");

		m_env_map = Load_KTX("./resources/equirectangularmap1.ktx");
		m_cube_map = Load_KTX("./resources/mountaincube.ktx");

		m_random.Init();

		glGenBuffers(1, &m_ubo);
		m_data = new Material_Uniform;

		glGenVertexArrays(1, &m_skybox_vao);
		glBindVertexArray(m_skybox_vao);


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

		m_mesh_rotation = glm::rotate(cos((float)m_time * 0.1f), glm::vec3(0.0, 1.0, 0.0)) * glm::scale(glm::vec3(15.0f, 15.0f, 15.0f));

		Material_Uniform temp_material = { m_light_pos, 0.0, m_diffuse_albedo, 0.0, m_specular_albedo, m_specular_power, m_ambient, 0.0, m_rim_color, m_rim_power };
		memcpy(m_data, &temp_material, sizeof(Material_Uniform));
	}
	void OnDraw() {
		static const GLfloat one = 1.0f;
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClearBufferfv(GL_DEPTH, 0, &one);

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glEnable(GL_DEPTH_TEST);

		glUseProgram(m_skybox_program);
		//glBindVertexArray(m_skybox_vao);
		glBindTextureUnit(0, m_cube_map);
		glUniform3fv(0, 1, glm::value_ptr(m_camera.Eye()));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));
		glUniform3fv(3, 1, glm::value_ptr(glm::vec3(15.0f)));
		m_cube.OnDraw();

		/*glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);*/
		/*glEnable(GL_DEPTH_TEST);

		glUseProgram(m_normal_program);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_mesh_rotation));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_camera.m_view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(m_camera.m_proj));


		glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(Material_Uniform), m_data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo);

		glBindTextureUnit(0, m_tex_base);
		glBindTextureUnit(1, m_tex_normal);
		glBindTextureUnit(2, m_env_map);
		m_cube.OnDraw();*/
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
#endif //CUBE_MAP_REDUX