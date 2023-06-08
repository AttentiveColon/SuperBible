#include "Defines.h"
#ifdef BITMAP_FONTS
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"


static const GLchar* vs_source = R"(
#version 450 core

void main()
{
	gl_Position = vec4(float((gl_VertexID >> 1) & 1) * 2.0 - 1.0, float((gl_VertexID & 1)) * 2.0 - 1.0, 0.0, 1.0);
}
)";

static const GLchar* fs_source = R"(
#version 450 core

layout (origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform isampler2D text_buffer;
layout (binding = 1) uniform isampler2DArray font_texture;

void main()
{
	ivec2 frag_coord = ivec2(gl_FragCoord.xy);
	ivec2 char_size = textureSize(font_texture, 0).xy;
	ivec2 char_location = frag_coord / char_size;
	ivec2 texel_coord = frag_coord % char_size;
	int character = texelFetch(text_buffer, char_location, 0).x;
	float val = texelFetch(font_texture, ivec3(texel_coord, character), 0).x;
	
	if (val == 0.0)
		discard;
	color = vec4(1.0);
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vs_source, NULL},
	{GL_FRAGMENT_SHADER, fs_source, NULL},
	{GL_NONE, NULL, NULL}
};

struct Text
{
	GLuint vao;
	GLuint text_program;
	GLuint text_buffer;
	GLuint font_texture;
	int buffer_width, buffer_height;
	char* screen_buffer;
	bool dirty;
	int cursor_x = 0;
	int cursor_y = 0;
	void Init(int width, int height, const char* font)
	{
		buffer_width = width;
		buffer_height = height;
		text_program = LoadShaders(shader_text);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenTextures(1, &text_buffer);
		glBindTexture(GL_TEXTURE_2D, text_buffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, width, height);

		font_texture = Load_KTX(font);

		screen_buffer = new char[width * height];
		memset(screen_buffer, 0, width * height);
	}
	void Clear()
	{
		memset(screen_buffer, 0, buffer_width * buffer_height);
		dirty = true;
		cursor_x = 0;
		cursor_y = 0;
	}
	void MoveCursor(int x, int y) 
	{
		cursor_x = x;
		cursor_y = y;
	}
	void Scroll(int lines)
	{
		const char* src = screen_buffer + lines * buffer_width;
		char* dst = screen_buffer;

		memmove(dst, src, (buffer_height - lines) * buffer_width);
		dirty = true;
	}
	void Print(const char* str)
	{
		const char* p = str;
		char c;
		char* dst = screen_buffer + cursor_y * buffer_width + cursor_x;

		while (*p != 0) {
			c = *p++;
			if (c == '\n') {
				cursor_y++;
				cursor_x = 0;
				if (cursor_y >= buffer_height) {
					cursor_y--;
					Scroll(1);
				}
				dst = screen_buffer + cursor_y * buffer_width + cursor_x;
			}
			else {
				*dst++ = c;
				cursor_x++;
				if (cursor_x >= buffer_width) {
					cursor_y++;
					cursor_x = 0;
					if (cursor_y >= buffer_height) {
						cursor_y--;
						Scroll(1);
					}
					dst = screen_buffer + cursor_y * buffer_width + cursor_x;
				}
			}
		}
		dirty = true;
	}
	void Draw()
	{
		glUseProgram(text_program);
		glBindTextureUnit(0, text_buffer);
		if (dirty)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer_width, buffer_height, GL_RED_INTEGER, GL_UNSIGNED_BYTE, screen_buffer);
			dirty = false;
		}
		glBindTextureUnit(1, font_texture);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
};

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	Text m_text;


	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		//Create Strings
		static const char string[] = "The quick brown fox jumped over the lazy dog.";

		//Initialize text struct
		m_text.Init(32, 64, "./resources/cp437_9x16.ktx");

		//Draw to text buffer
		m_text.Print(string);
		m_text.Print("\n");
		m_text.Print(string);
		m_text.Print("\n");
		m_text.Print(string);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();
	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		//Print text
		m_text.Draw();
	}
	void OnGui() {
		ImGui::Begin("User Defined Settings");
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Time: %f", m_time);
		ImGui::ColorEdit4("Clear Color", m_clear_color);
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
#endif //BITMAP_FONTS