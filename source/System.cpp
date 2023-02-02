#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "System.h"
#include <locale>
#include <codecvt>
#include <limits>
#undef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

//-------------------------------------------------------------------------------------------------
// SYSTEM
//-------------------------------------------------------------------------------------------------

System::System(SystemConf config)
	:m_window(config),
	m_input(),
	m_audio()
{
#ifdef _DEBUG
	glfwSetErrorCallback(Error_Callback);
#endif //_DEBUG

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW!" << std::endl;
		exit(-1);
	}

#ifdef _DEBUG
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	std::cout << "Renderer: " << renderer << std::endl;
	std::cout << "OpenGL version supported: " << version << std::endl;
#endif //_DEBUG

	glfwSetWindowUserPointer(m_window.m_handle, this);

	glfwSetKeyCallback(m_window.m_handle, Key_Callback);
	glfwSetCharCallback(m_window.m_handle, Character_Callback);
	glfwSetFramebufferSizeCallback(m_window.m_handle, Framebuffer_Size_Callback);
	glfwSetWindowPosCallback(m_window.m_handle, Window_Pos_Callback);
	glfwSetCursorPosCallback(m_window.m_handle, Cursor_Pos_Callback);
	glfwSetCursorEnterCallback(m_window.m_handle, Cursor_Enter_Callback);
	glfwSetMouseButtonCallback(m_window.m_handle, Mouse_Button_Callback);
	glfwSetScrollCallback(m_window.m_handle, Mouse_Scroll_Callback);
}

void System::Key_Callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	System* system = static_cast<System*>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS) {
		system->m_input.SetKey(key);
	}
	if (action == GLFW_RELEASE) {
		system->m_input.ReleaseKey(key);
	}
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		system->m_window.TerminateProgram();
	}
}

void System::Character_Callback(GLFWwindow* window, unsigned int codepoint)
{
	System* system = static_cast<System*>(glfwGetWindowUserPointer(window));
	if (system->m_input.m_charsActive) {
		system->m_input.GetChar(codepoint);
	}
}

void System::Error_Callback(int code, const char* description)
{
	std::cerr << "GLFW ERROR CODE: " << code << ": " << description << std::endl;
}

void System::Framebuffer_Size_Callback(GLFWwindow* window, i32 width, i32 height)
{
	glViewport(0, 0, width, height);
#ifdef _DEBUG
	std::cout << "Framebuffer size changed:" << std::endl;
	std::cout << "\tWidth:  " << width << std::endl;
	std::cout << "\tHeight: " << height << std::endl;
#endif //_DEBUG
}

void System::Window_Pos_Callback(GLFWwindow* window, i32 xPos, i32 yPos)
{
	System* system = static_cast<System*>(glfwGetWindowUserPointer(window));
	system->m_window.m_xPos = xPos;
	system->m_window.m_yPos = yPos;
}

void System::Cursor_Pos_Callback(GLFWwindow* window, f64 xPos, f64 yPos)
{
	System* system = static_cast<System*>(glfwGetWindowUserPointer(window));
	system->m_input.m_mouseXpos = xPos;
	system->m_input.m_mouseYpos = yPos;
}

void System::Cursor_Enter_Callback(GLFWwindow* window, int entered)
{
	System* system = static_cast<System*>(glfwGetWindowUserPointer(window));
	if (entered) {
		system->m_input.m_mouseActive = true;
	}
	else {
		system->m_input.m_mouseActive = false;
	}
}

void System::Mouse_Button_Callback(GLFWwindow* window, int button, int action, int mods)
{
	System* system = static_cast<System*>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS) {
		system->m_input.SetMouse(button);
	}
	if (action == GLFW_RELEASE) {
		system->m_input.ReleaseMouse(button);
	}
}

void System::Mouse_Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset)
{
	System* system = static_cast<System*>(glfwGetWindowUserPointer(window));
	system->m_input.MouseScroll(xoffset, yoffset);
}

//-------------------------------------------------------------------------------------------------
// WINDOW
//-------------------------------------------------------------------------------------------------

Window::Window(SystemConf config)
	:m_width(config.width),
	m_height(config.height),
	m_xPos(config.pos_x),
	m_yPos(config.pos_y),
	m_running(true),
	m_framecount(0),
	m_frametime(0),
	m_framelimit(1.0 / config.frame_limit),
	m_fps(0),
	m_time(0.0)
{
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW!" << std::endl;
		exit(-1);
	}

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);

	GLFWwindow* handle;

	if (config.windowed_fullscreen) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		handle = glfwCreateWindow(mode->width, mode->height, config.window_title, monitor, NULL);
		m_xPos = 50;
		m_yPos = 50;
	}
	else {
		handle = glfwCreateWindow(config.width, config.height, config.window_title, NULL, NULL);
		glfwSetWindowPos(handle, config.pos_x, config.pos_y);
	}

	if (!handle) {
		std::cerr << "GLFW window creation failed!" << std::endl;
		glfwTerminate();
		exit(-1);
	}

	//set vsync
	if (config.vsync) {
		glfwSwapInterval(1);
	}
	else {
		glfwSwapInterval(0);
	}

	//set icon
	Window::SetIcon(handle, config.icon_path);

	glfwMakeContextCurrent(handle);
	this->m_handle = handle;
}

Window::~Window()
{
	glfwDestroyWindow(m_handle);
	glfwTerminate();
}

void Window::ResizeWindow(i32 width, i32 height)
{
	m_width = width;
	m_height = height;
	glfwSetWindowSize(m_handle, width, height);
}

void Window::SetWindow(i32 xPos, i32 yPos, i32 width, i32 height)
{
	m_xPos = xPos;
	m_yPos = yPos;
	m_width = width;
	m_height = height;
	glfwSetWindowMonitor(m_handle, NULL, m_xPos, m_yPos, m_width, m_height, GLFW_DONT_CARE);
}


void Window::FullScreen(GLFWmonitor* monitor, i32 width, i32 height)
{
	glfwSetWindowMonitor(m_handle, monitor, 0, 0, width, height, GLFW_DONT_CARE);
}

void Window::ExitFullScreen()
{
	if (m_xPos == 0 || m_yPos == 0) {
		m_xPos = 50;
		m_yPos = 50;
	}
	glfwSetWindowMonitor(m_handle, NULL, m_xPos, m_yPos, m_width, m_height, GLFW_DONT_CARE);
}

void Window::TerminateProgram()
{
	m_running = false;
}

void Window::ShowCursor(bool cursor_on)
{
	if (cursor_on) {
		glfwSetInputMode(m_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	else {
		glfwSetInputMode(m_handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
}

void Window::SetIcon(GLFWwindow* window, const char* filePath)
{
	GLFWimage image = rgba_bmp_load(filePath);
	if (image.pixels == nullptr) {
		std::cerr << "Icon loading failed: '" << filePath << "'" << std::endl;
		return;
	}
	else {
		glfwSetWindowIcon(window, 1, &image);
	}
	delete[] image.pixels;
}

void Window::GetWindowStats()
{
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	std::cout << "width:      " << mode->width << std::endl;
	std::cout << "height:     " << mode->height << std::endl;
	std::cout << "red-bits:   " << mode->redBits << std::endl;
	std::cout << "green-bits: " << mode->greenBits << std::endl;
	std::cout << "blue-bits:  " << mode->blueBits << std::endl;
	std::cout << "refresh:    " << mode->refreshRate << std::endl;
}

void Window::UpdateFPS()
{
	f64 current_time = glfwGetTime();
	m_framecount++;
	if (current_time - m_frametime >= 1.0) {
		m_fps = m_framecount;
		m_framecount = 0;
		m_frametime = current_time;
	}
}

void Window::UpdateTime(f64 dt)
{
	m_time += dt;
}

void Window::VSync(bool state)
{
	if (state) {
		glfwSwapInterval(1);
	}
	else {
		glfwSwapInterval(0);
	}
}

void Window::SetFrameLimit(f64 limit)
{
	m_framelimit = 1.0 / limit;
}

//-------------------------------------------------------------------------------------------------
// INPUT
//-------------------------------------------------------------------------------------------------

void Input::SetKey(i32 key) {
	m_state[key] = true;
}

void Input::ReleaseKey(i32 key) {
	m_state[key] = false;
}

void Input::AdvanceInput()
{
	memcpy(m_prevState, m_state, sizeof(m_state));
	memcpy(m_mouse_prevState, m_mouse_state, sizeof(m_mouse_state));
	m_mouseXposPrev = m_mouseXpos;
	m_mouseYposPrev = m_mouseYpos;
	m_mouse_scrollHorizontal = 0.0;
	m_mouse_scrollVertical = 0.0;
}

bool Input::Pressed(i32 key)
{
	return (m_state[key] && !m_prevState[key]);
}

bool Input::Held(i32 key)
{
	return m_state[key];
}

bool Input::Released(i32 key)
{
	return (!m_state[key] && m_prevState[key]);
}

void Input::ActivateChars()
{
	m_charsActive = true;
	m_chars.clear();
}

void Input::DeactivateChar()
{
	m_charsActive = false;
}

void Input::GetChar(u32 codepoint)
{
	const char32_t cp = (const char32_t)codepoint;
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
	m_chars += convert.to_bytes(&cp, &cp + 1);
}

bool Input::HasChars()
{
	if (!m_chars.empty()) return true;
	return false;
}

std::string Input::Chars()
{
	std::string chars = m_chars;
	m_chars.clear();
	return chars;
}

MousePos Input::GetMousePos()
{
	MousePos mouse;
	mouse.x = m_mouseXpos;
	mouse.y = m_mouseYpos;
	return mouse;
}

bool Input::MouseIsActive()
{
	return m_mouseActive;
}

void Input::SetRawMouseMode(GLFWwindow* window)
{
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (glfwRawMouseMotionSupported()) {
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
}

void Input::UnsetRawMouseMode(GLFWwindow* window)
{
	if (glfwRawMouseMotionSupported()) {
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
	}
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

MousePos Input::GetMouseRaw()
{
	MousePos mouse;
	mouse.x = m_mouseXpos - m_mouseXposPrev;
	mouse.y = m_mouseYpos - m_mouseYposPrev;
	return mouse;
}

bool Input::MousePressed(i32 button)
{
	return (m_mouse_state[button] && !m_mouse_prevState[button]);
}

bool Input::MouseHeld(i32 button)
{
	return m_mouse_state[button];
}

bool Input::MouseReleased(i32 button)
{
	return (!m_mouse_state[button] && m_mouse_prevState[button]);
}

bool Input::MouseScrollVertical()
{
	return (m_mouse_scrollVertical != 0);
}

double Input::GetMouseScrollVertical()
{
	return m_mouse_scrollVertical;
}

bool Input::MouseScrollHorizontal()
{
	return (m_mouse_scrollHorizontal != 0);
}

double Input::GetMouseScrollHorizontal()
{
	return m_mouse_scrollHorizontal;
}

void Input::SetMouse(i32 button)
{
	m_mouse_state[button] = true;
}

void Input::ReleaseMouse(i32 button)
{
	m_mouse_state[button] = false;
}

void Input::MouseScroll(double xoffset, double yoffset)
{
	m_mouse_scrollHorizontal = xoffset;
	m_mouse_scrollVertical = yoffset;
}

//-------------------------------------------------------------------------------------------------
// AUDIO
//-------------------------------------------------------------------------------------------------

Audio::Audio()
{
	m_engine = createIrrKlangDevice();
	if (!m_engine) {
		std::cerr << "Failed to initialize sound engine" << std::endl;
		throw;
	}
}

Audio::~Audio()
{
	m_engine->drop();
}

void Audio::PlayOneShot(const char* file)
{
	m_engine->play2D(file);
}

ISound* Audio::PlayLoop(const char* file)
{
	return m_engine->play2D(file, true, false, true);
}

void Audio::StopLoop(ISound* audio_track_ptr)
{
	audio_track_ptr->stop();
	audio_track_ptr->drop();
}

//-------------------------------------------------------------------------------------------------
// Random
//-------------------------------------------------------------------------------------------------

void Random::Init()
{
	s_RandomEngine.seed(std::random_device()());
}

float Random::Float()
{
	//return (float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<unsigned int>::max();
	return (float)s_Distribution(s_RandomEngine) / (float)U32_MAX;
}

i32 Random::IntRange(i32 low, i32 high)
{
#ifdef _DEBUG
	assert(low < high);
#endif //_DEBUG

	return (static_cast<i32>((high - low) * Random::Float())) + low;
}

std::mt19937 Random::s_RandomEngine;
std::uniform_int_distribution<std::mt19937::result_type> Random::s_Distribution;

//-------------------------------------------------------------------------------------------------
// EVENT
//-------------------------------------------------------------------------------------------------

void OpenGL_Debug_Init();
void ImGui_Init(GLFWwindow* window);
void ImGui_StartFrame();
void ImGui_RenderFrame();

void Event::Run(System* system, Program& program)
{
	Window& window = system->m_window;
	Input& input = system->m_input;
	Audio& audio = system->m_audio;

#ifdef _DEBUG
	OpenGL_Debug_Init();
	ImGui_Init(window.m_handle);
#endif //_DEBUG
	
	Random::Init();
	program.OnInit(input, audio, window);

	f64 ticks = glfwGetTime();
	while (!glfwWindowShouldClose(window.m_handle) && window.IsRunning()) {
		glfwPollEvents();

		//Calculate DeltaTime
		while (glfwGetTime() - ticks < window.GetFrameLimit()) {}
		f64 dt = glfwGetTime() - ticks;
		ticks = glfwGetTime();

		window.UpdateFPS();
		window.UpdateTime(dt);

		program.OnUpdate(input, audio, window, dt);
		program.OnDraw();

#ifdef _DEBUG
		ImGui_StartFrame();
		program.OnGui();
		ImGui_RenderFrame();
#endif //_DEBUG

		input.AdvanceInput();
		glfwSwapBuffers(system->m_window.m_handle);
	}
}

void OpenGL_Debug_Init()
{
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(Debug_Callback, nullptr);
}

void ImGui_Init(GLFWwindow* window)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450");
}

void ImGui_StartFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGui_RenderFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}