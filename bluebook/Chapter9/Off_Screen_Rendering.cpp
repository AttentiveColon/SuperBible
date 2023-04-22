#include "Defines.h"
#ifdef OFF_SCREEN_RENDERING
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"


static const GLchar* vertex_shader_source = R"(
#version 450 core

layout (location = 0)
in vec3 position;
layout (location = 1)
in vec3 normal;
layout (location = 2)
in vec2 uv;

layout (location = 12)
uniform mat4 mvp;
layout (location = 13)
uniform vec3 model_pos;

out VS_OUT
{	
	vec3 normal;
	vec2 uv;
} vs_out;

void main()
{
	gl_Position = mvp * (vec4(position, 1.0) + vec4(model_pos, 1.0));
	vs_out.normal = normal;
	vs_out.uv = uv;
}
)";

static const GLchar* fragment_shader_source = R"(
#version 450 core

out vec4 color;

in VS_OUT
{
	vec3 normal;
	vec2 uv;
} fs_in;

void main()
{
	color = vec4(fs_in.uv.x, fs_in.uv.y, 0.0, 1.0);
}
)";

static const GLchar* fragment_shader_source2 = R"(
#version 450 core

out vec4 color;

uniform sampler2D u_tex;

in VS_OUT
{
	vec3 normal;
	vec2 uv;
} fs_in;

void main()
{
	color = texture(u_tex, fs_in.uv);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source, NULL},
	{GL_NONE, NULL, NULL}
};

static ShaderText shader_text2[] = {
	{GL_VERTEX_SHADER, vertex_shader_source, NULL},
	{GL_FRAGMENT_SHADER, fragment_shader_source2, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLenum blend_func[] = {
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_CONSTANT_COLOR,
	GL_ONE_MINUS_CONSTANT_COLOR,
	GL_CONSTANT_ALPHA,
	GL_ONE_MINUS_CONSTANT_ALPHA,
	GL_SRC_ALPHA_SATURATE,
	GL_SRC1_COLOR,
	GL_ONE_MINUS_SRC1_COLOR,
	GL_SRC1_ALPHA,
	GL_ONE_MINUS_SRC1_ALPHA
};

static const int num_blend_funcs = sizeof(blend_func) / sizeof(blend_func[0]);
static const float x_scale = 20.0f / float(num_blend_funcs);
static const float y_scale = 16.0f / float(num_blend_funcs);

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_program;
	GLuint m_program2;
	GLuint m_frame_buf;
	GLuint m_color_tex;

	SB::Camera m_camera;
	bool m_input_mode = false;


	bool m_wireframe = false;
	ObjMesh m_cube;
	ObjMesh m_monkey;
	glm::vec3 m_cube_pos = glm::vec3(0.0f, 0.0f, 0.0f);

	bool m_masks[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };

	bool m_flip = true;


	Application()
		:m_clear_color{ 0.6f, 0.4f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_CULL_FACE);

		//Create frame buffer object
		glCreateFramebuffers(1, &m_frame_buf);
		glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buf);

		//Create texture
		glGenTextures(1, &m_color_tex);
		glBindTexture(GL_TEXTURE_2D, m_color_tex);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, window.GetWindowDimensions().width, window.GetWindowDimensions().height);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Attach texture to frame buffer object
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_color_tex, 0);

		//Tell opengl which buffers I want to write to (in this case just the color buffer)
		static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, draw_buffers);

		m_program = LoadShaders(shader_text);
		m_program2 = LoadShaders(shader_text2);

		m_cube.Load_OBJ("./resources/cube.obj");
		m_monkey.Load_OBJ("./resources/monkey.obj");

		m_camera = SB::Camera("Camera", glm::vec3(1.0f, 2.0f, 5.0f), glm::vec3(1.0f, 1.0f, 0.0f), SB::CameraType::Perspective, 16.0 / 9.0, 0.90, 1.1, 1000.0);
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
	}
	void OnDraw() {
		//Bind the frame buffer object and clear the buffer
		glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buf);
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		//Render the scene and draw all data to the fbo color texture
		glUseProgram(m_program);
		RenderScene();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearBufferfv(GL_COLOR, 0, m_clear_color);
		glClear(GL_DEPTH_BUFFER_BIT);

		//After clearing the buffer again, bind our fbo texture we drew to and re-render the scene with the normal back buffer
		//Cubes should now be textured with the current scene view
		glBindTexture(GL_TEXTURE_2D, m_color_tex);
		glUseProgram(m_program2);
		RenderScene();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
		ImGui::Checkbox("Wireframe", &m_wireframe);
		ImGui::Checkbox("Red Mask", &m_masks[0]);
		ImGui::Checkbox("Green Mask", &m_masks[1]);
		ImGui::Checkbox("Blue Mask", &m_masks[2]);
		ImGui::Checkbox("Alpha Mask", &m_masks[3]);
		ImGui::End();
	}

	void RenderScene() {
		glColorMask(m_masks[0], m_masks[1], m_masks[2], m_masks[3]);

		if (m_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		glUniformMatrix4fv(12, 1, GL_FALSE, glm::value_ptr(m_camera.ViewProj()));

		glEnable(GL_BLEND);
		glBlendColor(0.2f, 0.5f, 0.7f, 0.5f);
		for (int j = 0; j < num_blend_funcs; ++j) {
			for (int i = 0; i < num_blend_funcs; ++i) {
				m_cube_pos = glm::vec3(x_scale * float(i) * 4.0, y_scale * float(j) * 4.0, -25.0f);
				glUniform3fv(13, 1, glm::value_ptr(m_cube_pos));
				glBlendFunc(blend_func[i], blend_func[j]);
				if (m_flip) {
					m_cube.OnDraw();
					m_flip = !m_flip;
				}
				else {
					m_monkey.OnDraw();
					m_flip = !m_flip;
				}
			}
		}
		glColorMask(true, true, true, true);
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
#endif //OFF_SCREEN_RENDERING
