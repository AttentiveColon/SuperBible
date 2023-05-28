#include "Defines.h"
#ifdef PACKET_BUFFER
#include "System.h"
#include "Texture.h"
#include "Model.h"
#include "Mesh.h"
#include <chrono>
#include <omp.h>

static const GLchar* vs_source = R"(
#version 450 core

layout (binding = 0) uniform BLOCK
{
	vec4 vtx_color[4];
};

out vec4 vs_fs_color;

void main()
{
	vs_fs_color = vtx_color[gl_VertexID & 3];
	gl_Position = vec4((gl_VertexID & 2) - 1.0, (gl_VertexID & 1) * 2.0 - 1.0, 0.5, 1.0);
}
)";

static const GLchar* fs_source = R"(
#version 450 core

layout (location = 0) out vec4 color;

in vec4 vs_fs_color;

void main()
{
	color = vs_fs_color;
}
)";

static ShaderText shader_text[] = {
	{GL_VERTEX_SHADER, vs_source, NULL},
	{GL_FRAGMENT_SHADER, fs_source, NULL},
	{GL_NONE, NULL, NULL}
};

static const GLfloat colors[] = {
	1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 1.0f, 1.0f,
};

namespace packet
{
	struct base;

	typedef void (APIENTRY PFN_EXECUTE)(const base* __restrict pParams);

	struct base
	{
		PFN_EXECUTE pfnExecute;
	};

	struct BIND_PROGRAM : public base
	{
		GLuint program;

		static void APIENTRY execute(const BIND_PROGRAM* __restrict pParams)
		{
			glUseProgram(pParams->program);
		}
	};

	struct BIND_VERTEX_ARRAY : public base
	{
		GLuint vao;

		static void APIENTRY execute(const BIND_VERTEX_ARRAY* __restrict pParams)
		{
			glBindVertexArray(pParams->vao);
		}
	};

	struct BIND_BUFFER_RANGE : public base
	{
		GLenum target;
		GLuint index;
		GLuint buffer;
		GLintptr offset;
		GLsizeiptr size;

		static void APIENTRY execute(const BIND_BUFFER_RANGE* __restrict pParams)
		{
			glBindBufferRange(pParams->target, pParams->index, pParams->buffer, pParams->offset, pParams->size);
		}
	};

	struct DRAW_ARRAYS : public base
	{
		GLenum mode;
		GLint first;
		GLsizei count;
		GLsizei primcount;
		GLuint baseinstance;

		static void APIENTRY execute(const DRAW_ARRAYS* __restrict pParams)
		{
			glDrawArraysInstancedBaseInstance(pParams->mode, pParams->first, pParams->count, pParams->primcount, pParams->baseinstance);
		}
	};

	union ALL_PACKETS
	{
	public:
		PFN_EXECUTE execute;
	private:
		base Base;
		BIND_PROGRAM BindProgram;
		BIND_VERTEX_ARRAY BindVertexArray;
		DRAW_ARRAYS DrawArrays;
	};
}

struct packet_stream
{
	unsigned int max_packets;
	packet::ALL_PACKETS* m_packets;
	unsigned int num_packets;

	packet_stream() : max_packets(0), m_packets(nullptr), num_packets(0) { }

	void Init(int max_packets_);
	void execute();

	inline void BindProgram(GLuint program);
	inline void BindVertexArray(GLuint vao);
	inline void BindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	inline void DrawArrays(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance);

	template <typename T>
	T* NextPacket() { return reinterpret_cast<T*>(&m_packets[num_packets++]); }
};

void packet_stream::Init(int max_packets_)
{
	max_packets = max_packets_;
	num_packets = 0;
	m_packets = new packet::ALL_PACKETS[max_packets];
	memset(m_packets, 0, max_packets * sizeof(packet::ALL_PACKETS));
}

void packet_stream::execute()
{
	const packet::ALL_PACKETS* pPacket;

	if (!num_packets)
		return;

	for (pPacket = m_packets; pPacket->execute != nullptr; pPacket++)
	{
		pPacket->execute((packet::base*)pPacket);
	}
}

void packet_stream::BindProgram(GLuint program)
{
	packet::BIND_PROGRAM* __restrict pPacket = NextPacket<packet::BIND_PROGRAM>();

	pPacket->pfnExecute = packet::PFN_EXECUTE(packet::BIND_PROGRAM::execute);
	pPacket->program = program;
}

void packet_stream::BindVertexArray(GLuint vao)
{
	packet::BIND_VERTEX_ARRAY* __restrict pPacket = NextPacket<packet::BIND_VERTEX_ARRAY>();

	pPacket->pfnExecute = packet::PFN_EXECUTE(packet::BIND_VERTEX_ARRAY::execute);
	pPacket->vao = vao;
}

void packet_stream::BindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
	packet::BIND_BUFFER_RANGE* __restrict pPacket = NextPacket<packet::BIND_BUFFER_RANGE>();

	pPacket->pfnExecute = packet::PFN_EXECUTE(packet::BIND_BUFFER_RANGE::execute);
	pPacket->target = target;
	pPacket->index = index;
	pPacket->buffer = buffer;
	pPacket->offset = offset;
	pPacket->size = size;
}

void packet_stream::DrawArrays(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance)
{
	packet::DRAW_ARRAYS* __restrict pPacket = NextPacket<packet::DRAW_ARRAYS>();

	pPacket->pfnExecute = packet::PFN_EXECUTE(packet::DRAW_ARRAYS::execute);
	pPacket->mode = mode;
	pPacket->first = first;
	pPacket->count = count;
	pPacket->primcount = primcount;
	pPacket->baseinstance = baseinstance;
}

struct Application : public Program {
	float m_clear_color[4];
	u64 m_fps;
	f64 m_time;

	GLuint m_vao;
	GLuint m_program;
	GLuint m_buffer_block;

	packet_stream stream;

	Application()
		:m_clear_color{ 0.1f, 0.1f, 0.1f, 1.0f },
		m_fps(0),
		m_time(0.0)
	{}

	void OnInit(Input& input, Audio& audio, Window& window) {
		//Init Stream
		stream.Init(256);
		

		m_program = LoadShaders(shader_text);
		glGenVertexArrays(1, &m_vao);
		

		glGenBuffers(1, &m_buffer_block);
		glBindBuffer(GL_UNIFORM_BUFFER, m_buffer_block);
		glBufferStorage(GL_UNIFORM_BUFFER, sizeof(colors), colors, 0);


		stream.BindProgram(m_program);
		stream.BindBufferRange(GL_UNIFORM_BUFFER, 0, m_buffer_block, 0, sizeof(colors));
		stream.BindVertexArray(m_vao);
		stream.DrawArrays(GL_TRIANGLES, 0, 4, 1, 0);
	}
	void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) {
		m_fps = window.GetFPS();
		m_time = window.GetTime();

	}
	void OnDraw() {
		glViewport(0, 0, 1600, 900);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		stream.execute();

		/*glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_buffer_block, 0, sizeof(colors));
		glDrawArrays(GL_TRIANGLES, 0, 12);*/
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
#endif //PACKET_BUFFER