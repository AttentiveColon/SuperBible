#pragma once

#include "Defines.h"

#ifndef _DEBUG
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif //!_DEBUG

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "GL_Helpers.h"

#include <iostream>
#include <irrKlang.h>
using namespace irrklang;

#include <random>

struct GLFWwindow;
struct SystemConf;

//-------------------------------------------------------------------------------------------------
// WINDOW
//-------------------------------------------------------------------------------------------------

struct Window {
	friend struct System;
	friend struct Event;

	Window(SystemConf config);
	~Window();

	void ResizeWindow(i32 width, i32 height);
	void SetWindow(i32 xPos, i32 yPos, i32 width, i32 height);
	void FullScreen(GLFWmonitor* monitor, i32 width, i32 height);
	void ExitFullScreen();
	void TerminateProgram();

	static void SetIcon(GLFWwindow* window, const char* filePath);
	void ShowCursor(bool cursor_on);
	void GetWindowStats();
	void VSync(bool state);
	void SetFrameLimit(f64 limit);

	u64 GetFPS() { return m_fps; }
	f64 GetTime() { return m_time; }
	f64 GetFrameLimit() { return m_framelimit; }
	bool IsRunning() { return m_running; }
	GLFWwindow* GetHandle() { return m_handle; }
	WindowXY GetWindowDimensions() { return WindowXY{ m_width, m_height }; }

private:
	void UpdateFPS();
	void UpdateTime(f64 dt);

private:
	GLFWwindow* m_handle;
	i32 m_xPos;
	i32 m_yPos;
	bool m_running;
	f64 m_framelimit;
	i32 m_width;
	i32 m_height;
	f64 m_frametime;
	u64 m_framecount;
	u64 m_fps;
	f64 m_time;
};

//-------------------------------------------------------------------------------------------------
// INPUT
//-------------------------------------------------------------------------------------------------


struct Input {
	friend struct System;
	friend struct Event;

	//Keyboard Input
public:
	bool Pressed(i32 key);
	bool Held(i32 key);
	bool Released(i32 key);

private:
	bool m_state[348] = { false };
	bool m_prevState[348] = { false };
	void SetKey(i32 key);
	void ReleaseKey(i32 key);
	void AdvanceInput();

	//Character Input
public:
	bool HasChars();
	std::string Chars();
	void ActivateChars();
	void DeactivateChar();

private:
	bool m_charsActive;
	std::string m_chars;
	void GetChar(u32 codepoint);


	//Mouse Input
public:
	MousePos GetMousePos();
	bool IsMouseActive();
	bool IsMouseRawActive();
	void SetRawMouseMode(GLFWwindow* window, bool active);
	void UnsetRawMouseMode(GLFWwindow* window);
	MousePos GetMouseRaw();
	bool MousePressed(i32 button);
	bool MouseHeld(i32 button);
	bool MouseReleased(i32 button);
	bool MouseScrollVertical();
	double GetMouseScrollVertical();
	bool MouseScrollHorizontal();
	double GetMouseScrollHorizontal();

private:
	bool m_mouseActive;
	bool m_mouse_state[8] = { false };
	bool m_mouse_prevState[8] = { false };
	bool m_raw_mouse = false;
	f64 m_mouseXpos;
	f64 m_mouseYpos;
	f64 m_mouseXposPrev;
	f64 m_mouseYposPrev;
	double m_mouse_scrollVertical;
	double m_mouse_scrollHorizontal;
	void SetMouse(i32 button);
	void ReleaseMouse(i32 button);
	void MouseScroll(double xoffset, double yoffset);
};

//-------------------------------------------------------------------------------------------------
// AUDIO
//-------------------------------------------------------------------------------------------------

struct Audio {
	Audio();
	~Audio();
	ISoundEngine* m_engine;

	void PlayOneShot(const char* file);
	ISound* PlayLoop(const char* file);
	void StopLoop(ISound* audio_track_ptr);
};

//-------------------------------------------------------------------------------------------------
// Random
//-------------------------------------------------------------------------------------------------

struct Random {
	static void Init();
	static f32 Float();
	static i32 IntRange(i32 low, i32 high);
private:
	static std::mt19937 s_RandomEngine;
	static std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;
};

//-------------------------------------------------------------------------------------------------
// SYSTEM
//-------------------------------------------------------------------------------------------------

struct SystemConf {
	i32 width;
	i32 height;
	i32 pos_x;
	i32 pos_y;
	const char* window_title;
	bool windowed_fullscreen;
	bool vsync;
	f64 frame_limit;
	const char* icon_path;
};

struct System
{
	System(SystemConf config);

	Window m_window;
	Input m_input;
	Audio m_audio;

	static void Key_Callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void Character_Callback(GLFWwindow* window, unsigned int codepoint);
	static void Error_Callback(int code, const char* description);
	static void Framebuffer_Size_Callback(GLFWwindow* window, i32 width, i32 height);
	static void Window_Pos_Callback(GLFWwindow* window, i32 xPos, i32 yPos);
	static void Cursor_Pos_Callback(GLFWwindow* window, f64 xPos, f64 yPos);
	static void Cursor_Enter_Callback(GLFWwindow* window, int entered);
	static void Mouse_Button_Callback(GLFWwindow* window, int button, int action, int mods);
	static void Mouse_Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset);
};

//-------------------------------------------------------------------------------------------------
// PROGRAM
//-------------------------------------------------------------------------------------------------

struct Program {
	virtual void OnInit(Input& input, Audio& audio, Window& window) = 0;
	virtual void OnUpdate(Input& input, Audio& audio, Window& window, f64 dt) = 0;
	virtual void OnDraw() = 0;
	virtual void OnGui() = 0;
};

//-------------------------------------------------------------------------------------------------
// EVENT
//-------------------------------------------------------------------------------------------------

struct Event
{
	static void Run(System* system, Program& program);
};

//-------------------------------------------------------------------------------------------------
// ENTRY POINT
//-------------------------------------------------------------------------------------------------

#ifdef _DEBUG
#define MAIN(config)					\
int main() {							\
	System system(config);				\
	Application app = Application();	\
	Event::Run(&system, app);			\
}
#else
#define MAIN(config)					\
int CALLBACK WinMain(					\
	_In_ HINSTANCE hInstance,			\
	_In_opt_ HINSTANCE hPrevInstance,	\
	_In_ LPSTR     lpCmdLine,			\
	_In_ int       nCmdShow				\
) {										\
	System system(config);				\
	Application app = Application();	\
	Event::Run(&system, app);			\
}
#endif //_DEBUG


