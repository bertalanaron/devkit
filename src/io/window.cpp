#include <devkit/io/window.h>
#include <devkit/io/input_combination.h>

#include <devkit/gfx/frame_buffer.h>

#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#undef main
#include "SDL2/SDL_syswm.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#pragma comment (lib, "Dwmapi")
#include <dwmapi.h>
#undef DELETE

struct dk::io::Window::Context {
	Window*            m_owner;
	SDL_Window*        m_window;
	Uint32             m_id;
	SDL_SysWMinfo      m_windowHandle;
	SDL_GLContext      m_glContext;
	long long unsigned m_lastTick = 0;
	bool               m_isOpen;
	ImGuiContext*      m_imguiContext;

	details::io::SDL_StaticContext* _m_staticContext;

	void initialize(const std::string& title, const glm::ivec2& size);
	void initializeImGui();
	void handleEvents();
	// Handle window specific events
	void handleEvent(SDL_WindowEvent event);
	void useContext();
	void close();
};

struct details::io::SDL_StaticContext : dk::common::SingletonBase<SDL_StaticContext> {
	void initialize();
	void handleEvents();
	bool isFirstContextInTick(long long unsigned contextLastHandledTick) const;
	auto ticks() { return m_ticks; }
	void insertWindow(Uint32 id, dk::io::Window::Context* windowContext);
	void eraseWindow(Uint32 id);

private:
	using IdToWindowMap = std::unordered_map<Uint32, dk::io::Window::Context*>;

	bool               m_initialized;
	long long unsigned m_ticks = 0;
	IdToWindowMap      m_windows;
};

void details::io::SDL_StaticContext::initialize()
{
	if (m_initialized)
		return;

	// Init SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	spdlog::trace("Initialized SDL");

	// Use OpenGL 3.3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	m_initialized = true;
}

void updateInputState() 
{
	// Button
	uint8_t button = SDL_GetMouseState(nullptr, nullptr);

	// Modkey
	uint8_t modkey = 0;
	SDL_Keymod mod = SDL_GetModState();
	if (mod & KMOD_SHIFT) modkey |= 1u << 0;
	if (mod & KMOD_CTRL)  modkey |= 1u << 1;
	if (mod & KMOD_ALT)   modkey |= 1u << 2;
	if (mod & KMOD_CAPS)  modkey |= 1u << 3;

	// Key
	uint64_t key = 0;
	const Uint8* keystate = SDL_GetKeyboardState(nullptr);
	// Numbers
	for (int i = 0; i < 10; ++i) {
		if (keystate[SDL_SCANCODE_0 + i]) 
			key |= (1ull << i);  // bits 0–9
	}
	// Letters
	for (int i = 0; i < 26; ++i) {
		if (keystate[SDL_SCANCODE_A + i])
			key |= (1ull << (10 + i));  // bits 10–35
	}

	details::io::commitInputState(button, modkey, key);
}

void details::io::SDL_StaticContext::handleEvents()
{
	++m_ticks;

	// Update mouse and keyboard state
	updateInputState();

	// Poll events
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type)
		{
		case SDL_WINDOWEVENT: {
			// Pass event to affected window
			auto it = m_windows.find(event.window.windowID);
			if (it != m_windows.end())
				it->second->handleEvent(event.window);
		} break;
		default:
			break;
		}

		// Pass event to ImGui
		ImGui_ImplSDL2_ProcessEvent(&event);
		// TODO: handle multiple windows
	}
}

bool details::io::SDL_StaticContext::isFirstContextInTick(long long unsigned contextLastHandledTick) const
{
	return contextLastHandledTick == m_ticks;
}

void details::io::SDL_StaticContext::insertWindow(Uint32 id, dk::io::Window::Context* windowContext)
{
	m_windows.insert({ id, windowContext });
}

void details::io::SDL_StaticContext::eraseWindow(Uint32 id)
{
	m_windows.erase(id);
}

void dk::io::Window::Context::initialize(const std::string& title, const glm::ivec2& size)
{
	details::io::SDL_StaticContext::instance().initialize();

	// Create window
	m_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		size.x, size.y, SDL_WINDOW_RESIZABLE | SDL_RENDERER_ACCELERATED);
	m_isOpen = true;
	// Get window id and insert window to global collection
	m_id = SDL_GetWindowID(m_window);
	details::io::SDL_StaticContext::instance().insertWindow(m_id, this);
	spdlog::trace("Created window: {}", (int)m_id);

	// Create opengl context
	m_glContext = SDL_GL_CreateContext(m_window);
	if (!m_glContext) {
		spdlog::error("OpenGL context could not be created! SDL Error: {}", SDL_GetError());
		std::terminate();
	}
	useContext();
	spdlog::trace("Created OpenGL context");
	
	// Disable V-Sync
	SDL_GL_SetSwapInterval(details::io::toUnderlying(m_owner->property<dk::io::properties::window::vsync>()));

	// Initialize GLEW after creating OpenGL context
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		spdlog::error("Error initializing GLEW!");
		std::terminate();
	}
	// Remove error caused by glewExperimental
	glGetError();
	spdlog::trace("Initialized GLEW");

	// Enable point size
	glEnable(GL_PROGRAM_POINT_SIZE);  

	// Get window handle
	SDL_VERSION(&m_windowHandle.version);
	SDL_GetWindowWMInfo(m_window, &m_windowHandle);

	// Initialize imgui context
	initializeImGui();
}

void dk::io::Window::Context::initializeImGui()
{
	m_imguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(m_imguiContext);

	ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
	ImGui_ImplOpenGL3_Init("#version 330");

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsClassic();
}

void dk::io::Window::Context::handleEvents()
{
	// Handle events once per tick (no need to handle events per each window)
	if (details::io::SDL_StaticContext::instance().isFirstContextInTick(m_lastTick))
		details::io::SDL_StaticContext::instance().handleEvents();
	m_lastTick = details::io::SDL_StaticContext::instance().ticks();
}

void dk::io::Window::Context::handleEvent(SDL_WindowEvent event)
{
	switch (event.event)
	{
	case SDL_WINDOWEVENT_CLOSE: 
		close(); 
		break;
	case SDL_WINDOWEVENT_RESIZED:
		m_owner->propertyChanged(dk::io::properties::window::size(event.data1, event.data2));
		break;
	default:
		break;
	}
}

void dk::io::Window::Context::useContext()
{
	SDL_GL_MakeCurrent(m_window, m_glContext);
}

void dk::io::Window::Context::close()
{
	m_isOpen = false;
	SDL_DestroyWindow(m_window);
	SDL_GL_DeleteContext(m_glContext);
	spdlog::trace("Closed window: {}", (int)m_id); 
}

void dk::io::Window::open()
{
	m_context->initialize(property<dk::io::properties::window::title>(), property<dk::io::properties::window::size>());
}

void dk::io::Window::close()
{
	m_context->close();
}

bool dk::io::Window::beginFrame()
{
	if (!m_context->m_isOpen)
		return false;

	// Update delta time
	auto now = std::chrono::system_clock::now();
	if (m_frameStart == std::chrono::system_clock::time_point())
		m_timeSinceLastFrame = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(0));
	else
		m_timeSinceLastFrame = (now - m_frameStart);
	m_frameStart = now;

	// Use window's SDL, OpenGL and ImGUI context
	useContext();

	// Update properties
	callPropertySetters();

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	// Handle events
	m_context->handleEvents();

	return true;
}

void dk::io::Window::endFrame()
{
	if (!m_context->m_isOpen)
		return;

	useContext();

	// Render ImGui draw data
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	ImGui::EndFrame();

	// Swap buffers
	SDL_GL_SwapWindow(m_context->m_window);
}

dk::io::Window::Window()
	: m_context(std::make_unique<Context>())
{
	m_context->_m_staticContext = &details::io::SDL_StaticContext::instance();
	m_context->m_owner = this;
}

void dk::io::Window::useContext()
{
	m_context->useContext();
	ImGui::SetCurrentContext(m_context->m_imguiContext);
	details::gfx::setBackbufferViewport(property<dk::io::properties::window::size>());
}

std::chrono::nanoseconds dk::io::Window::dt() const
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(m_timeSinceLastFrame);
}

dk::io::Window::~Window()
{
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::size& size) 
{
	SDL_SetWindowSize(window.m_context->m_window, size.x, size.y);
	spdlog::trace("Set size for window: {}", window.m_context->m_id);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::title& title)
{
	SDL_SetWindowTitle(window.m_context->m_window, title.c_str());
	spdlog::trace("Set title as \"{}\" for window: {}", title, window.m_context->m_id);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::border& border)
{
	bool isEnabled = border == dk::io::properties::window::border::enabled;
	SDL_SetWindowBordered(window.m_context->m_window, (SDL_bool)isEnabled);
	spdlog::trace("Set border {} for window: {}", (isEnabled ? "enabled" : "disabled"), window.m_context->m_id);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::theme& theme)
{
	BOOL USE_DARK_MODE = theme == dk::io::properties::window::theme::dark;
	BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
		window.m_context->m_windowHandle.info.win.window, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
		&USE_DARK_MODE, sizeof(USE_DARK_MODE)));
	spdlog::trace("Set {} theme for window: {}", (USE_DARK_MODE ? "dark" : "light"), window.m_context->m_id);

	// hack: Have to hide and show the window to apply color change
	bool border = window.property<dk::io::properties::window::border>() == dk::io::properties::window::border::enabled;
	SDL_SetWindowBordered(window.m_context->m_window, (SDL_bool)!border);
	SDL_SetWindowBordered(window.m_context->m_window, (SDL_bool)border);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::vsync& vsync)
{
	SDL_GL_SetSwapInterval(details::io::toUnderlying(vsync));
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::mode& mode)
{
	SDL_SetWindowFullscreen(window.m_context->m_window, details::io::toUnderlying(mode));
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::mouse_grab& mode)
{
	SDL_SetWindowMouseGrab(window.m_context->m_window, (SDL_bool)mode);
}

int details::io::toUnderlying(dk::io::properties::window::vsync vsync)
{
	switch (vsync)
	{
	case dk::io::properties::window::vsync::disabled: return 0;
	case dk::io::properties::window::vsync::retrace:  return 1;
	case dk::io::properties::window::vsync::adaptive: return -1;
	default:
		return 0;
	}
}

uint32_t details::io::toUnderlying(dk::io::properties::window::mode mode) {
	switch (mode)
	{
	case dk::io::properties::window::mode::windowed:   return 0;
	case dk::io::properties::window::mode::fullscreen: return SDL_WINDOW_FULLSCREEN_DESKTOP;
		break;
	default:
		break;
	}
	//SDL_WINDOW_FULLSCREEN, SDL_WINDOW_FULLSCREEN_DESKTOP
}
