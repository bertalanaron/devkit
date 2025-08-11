#include <devkit/io/window.h>
#include <devkit/io/input_combination.h>
#include <devkit/io/frame.h>

#include <devkit/gfx/frame_buffer.h>

#include <GL/glew.h>

#include <SDL3/SDL.h>
//#include <SDL3/SDL_main.h>
//#undef main
#include <SDL3/SDL_system.h>

#define DK_USE_IMGUI

#ifdef DK_USE_IMGUI
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h> // Needed for dock node access
#endif

#pragma comment (lib, "Dwmapi")
#include <dwmapi.h>
#undef DELETE

#if defined(SDL_PLATFORM_WIN32)
using WindowHandle = HWND;
#endif

struct dk::io::Window::Context {
	Window*            m_owner;
	SDL_Window*        m_window;
	Uint32             m_id;
	WindowHandle       m_windowHandle;
	SDL_GLContext      m_glContext = nullptr;
	long long unsigned m_lastTick = 0;
	bool               m_isOpen;
#ifdef DK_USE_IMGUI
	ImGuiContext*      m_imguiContext;
	ImGuiID            m_imguiDockspaceId;
#endif

	io::Frame          m_currentFrame;

	struct CursorWrapContext {
		bool wrappedLeft   = false;
		bool wrappedRight  = false;
		bool wrappedTop    = false;
		bool wrappedBottom = false;
	};

	gfx::Viewport     m_viewport;

	glm::vec2         m_cursorCurrentPos;
	glm::vec2         m_cursorPreviousPos;
	CursorWrapContext m_cursorWrapContext;

	details::io::SDL_StaticContext* _m_staticContext;

	void initialize(const std::string& title, const glm::ivec2& size, int msaa);
#ifdef DK_USE_IMGUI
	void initializeImGui();
#endif
	void handleEvents();
	// Handle window specific events
	void handleEvent(SDL_WindowEvent event);
	void useContext();
	void close();
	void prepareViewport();
};

struct details::io::SDL_StaticContext : dk::common::SingletonBase<SDL_StaticContext> {
	void initialize();
	void handleEvents();
	bool isFirstContextInTick(long long unsigned contextLastHandledTick) const;
	auto ticks() { return m_ticks; }
	void insertWindow(Uint32 id, dk::io::Window::Context* windowContext);
	void eraseWindow(Uint32 id);

	const auto& inputState() const
	{ return m_currentInputState; }

private:
	using IdToWindowMap = std::unordered_map<Uint32, dk::io::Window::Context*>;

	bool               m_initialized;
	long long unsigned m_ticks = 0;
	IdToWindowMap      m_windows;

	dk::io::InputState m_currentInputState;
};

void details::io::SDL_StaticContext::initialize()
{
	if (m_initialized)
		return;

	// Init SDL
	SDL_Init(SDL_INIT_VIDEO);
	spdlog::trace("Initialized SDL");

	// Use OpenGL 3.3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	m_initialized = true;
}

dk::io::InputState prepareInputState(float wheelDirection) 
{
	bool mouseCaptured = false;
	bool keyboardCaptured = false;
#ifdef DK_USE_IMGUI
	// Allow ImGui to capture mouse and keyboard
	auto& imguiIO = ImGui::GetIO();
	if (details::io::imguiDoCaptureMouse() && imguiIO.WantCaptureMouse)
		mouseCaptured = true;
	if (details::io::imguiDoCaptureKeyboard() && imguiIO.WantCaptureKeyboard)
		keyboardCaptured = true;
#endif

	dk::io::InputState state;

	if (!mouseCaptured) {
		// Button
		state.buttons = SDL_GetMouseState(nullptr, nullptr);

		// Wheel
		state.wheelDirection = wheelDirection;
		if (wheelDirection > 0) state.wheel |= (dk::io::wheel_t)dk::io::wheel_mask::up;
		if (wheelDirection < 0) state.wheel |= (dk::io::wheel_t)dk::io::wheel_mask::down;
	}

	if (!keyboardCaptured) {
		// Modkey
		SDL_Keymod mod = SDL_GetModState();
		if (mod & SDL_KMOD_SHIFT) state.modkeys |= (dk::io::modkey_t)dk::io::modkey_mask::shift;
		if (mod & SDL_KMOD_CTRL)  state.modkeys |= (dk::io::modkey_t)dk::io::modkey_mask::ctrl;
		if (mod & SDL_KMOD_ALT)   state.modkeys |= (dk::io::modkey_t)dk::io::modkey_mask::alt;
		if (mod & SDL_KMOD_CAPS)  state.modkeys |= (dk::io::modkey_t)dk::io::modkey_mask::caps;

		// Key
		const bool* keystate = SDL_GetKeyboardState(nullptr);
		// Numbers
		for (auto i = 0ull; i < 10; ++i) {
			if (keystate[SDL_SCANCODE_1 + i]) 
				state.keys |= ((dk::io::key_t)dk::io::key_mask::_0 << ((i + 1) % 10));
		}
		// Letters
		for (auto i = 0ull; i < 26; ++i) {
			if (keystate[SDL_SCANCODE_A + i])
				state.keys |= ((dk::io::key_t)dk::io::key_mask::a << i);
		}
		// Special
		if (keystate[SDL_SCANCODE_GRAVE])     state.keys |= (dk::io::key_t)dk::io::key_mask::grave;
		if (keystate[SDL_SCANCODE_ESCAPE])    state.keys |= (dk::io::key_t)dk::io::key_mask::esc;
		if (keystate[SDL_SCANCODE_TAB])       state.keys |= (dk::io::key_t)dk::io::key_mask::tab;
		if (keystate[SDL_SCANCODE_DELETE])    state.keys |= (dk::io::key_t)dk::io::key_mask::del;
		if (keystate[SDL_SCANCODE_EXECUTE])   state.keys |= (dk::io::key_t)dk::io::key_mask::enter;
		if (keystate[SDL_SCANCODE_BACKSPACE]) state.keys |= (dk::io::key_t)dk::io::key_mask::backspace;
		if (keystate[SDL_SCANCODE_BACKSLASH]) state.keys |= (dk::io::key_t)dk::io::key_mask::backslash;
	}

	return state;
}

bool isWindowEvent(SDL_Event const& e) 
{
	return e.type >= SDL_EVENT_WINDOW_FIRST && e.type <= SDL_EVENT_WINDOW_LAST;
}

void details::io::SDL_StaticContext::handleEvents()
{
	++m_ticks;

	float mouseWheelY = 0;

	// Poll events
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type)
		{
		case SDL_EVENT_MOUSE_WHEEL: {
			SDL_MouseWheelEvent& wheenEvent = event.wheel;
			mouseWheelY = wheenEvent.y;
			break;
		}
		default:
			break;
		}

		if (isWindowEvent(event)) {
			// Pass event to affected window
			auto it = m_windows.find(event.window.windowID);
			if (it != m_windows.end())
				it->second->handleEvent(event.window);
		}

#ifdef DK_USE_IMGUI
		// Pass event to ImGui
		ImGui_ImplSDL3_ProcessEvent(&event);
#endif
		// TODO: handle multiple windows
	}

	// Update mouse and keyboard state
	m_currentInputState = prepareInputState(mouseWheelY);
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

void dk::io::Window::Context::initialize(const std::string& title, const glm::ivec2& size, int msaa)
{
	details::io::SDL_StaticContext::instance().initialize();

	// Set up MSAA
	if (msaa > 1) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
	}

	// Create window
	m_window = SDL_CreateWindow(title.c_str(), /*SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,*/
		size.x, size.y, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL /*| SDL_RENDERER_ACCELERATED*/);
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
#if defined(SDL_PLATFORM_WIN32)
	m_windowHandle = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif

#ifdef DK_USE_IMGUI
	// Initialize imgui context
	initializeImGui();
#endif
}

#ifdef DK_USE_IMGUI
void dk::io::Window::Context::initializeImGui()
{
	m_imguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(m_imguiContext);

	ImGui_ImplSDL3_InitForOpenGL(m_window, m_glContext);
	ImGui_ImplOpenGL3_Init("#version 330");

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();
}
#endif

void dk::io::Window::Context::handleEvents()
{
	// Handle events once per tick (no need to handle events per each window)
	if (details::io::SDL_StaticContext::instance().isFirstContextInTick(m_lastTick))
		details::io::SDL_StaticContext::instance().handleEvents();
	m_lastTick = details::io::SDL_StaticContext::instance().ticks();

	// Update cursor state
	m_cursorPreviousPos = m_cursorCurrentPos;
	SDL_GetMouseState(&m_cursorCurrentPos.x, &m_cursorCurrentPos.y);
}

void dk::io::Window::Context::handleEvent(SDL_WindowEvent event)
{
	switch (event.type)
	{
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED: 
		close(); 
		break;
	case SDL_EVENT_WINDOW_RESIZED:
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
	SDL_GL_DestroyContext(m_glContext);
	spdlog::trace("Closed window: {}", (int)m_id); 
}

void dk::io::Window::Context::prepareViewport()
{
	
	ImGuiDockNode* node = ImGui::DockBuilderGetNode(m_imguiDockspaceId);
	if (!node)
		return;

	ImVec2 size = node->Size;  // Width/Height in pixels
	ImVec2 pos  = node->Pos;   // Top-left in screen space
	m_viewport = dk::gfx::Viewport(glm::ivec2(size.x, size.y), glm::ivec2(pos.x, pos.y));
}

void dk::io::Window::open(int msaa)
{
	m_context->initialize(property<dk::io::properties::window::title>(), property<dk::io::properties::window::size>(), msaa);
}

void dk::io::Window::close()
{
	m_context->close();
}

bool dk::io::Window::isOpen() const
{
	return m_context->m_isOpen;
}

const dk::io::Frame& dk::io::Window::beginFrame()
{
	DK_ASSERT((m_context->m_isOpen, "beginFrame requires the window to be open"));

	// Use window's SDL, OpenGL and ImGUI context
	useContext();

	// Update properties
	callPropertySetters();

	// Handle events
	m_context->handleEvents();

#ifdef DK_USE_IMGUI
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	m_context->m_imguiDockspaceId = ImGui::DockSpaceOverViewport(0, (const ImGuiViewport*)0, ImGuiDockNodeFlags_PassthruCentralNode);
#endif

	m_context->m_currentFrame = dk::io::Frame(m_context->m_currentFrame, 
		                                      m_context->m_viewport, 
		                                      m_context->_m_staticContext->inputState(),
		                                      m_context->m_cursorCurrentPos);
	m_context->m_currentFrame.makeCurrent();
	return m_context->m_currentFrame;
}

void dk::io::Window::endFrame()
{
	if (!m_context->m_isOpen)
		return;

	useContext();

#ifdef DK_USE_IMGUI
	dk::gfx::backBuffer().makeActive();

	// Render ImGui draw data
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	// ImGui multi viewports support
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}
#endif
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
#ifdef DK_USE_IMGUI
	ImGui::SetCurrentContext(m_context->m_imguiContext);
#endif
	details::gfx::setBackbufferViewport(property<dk::io::properties::window::size>());
}

bool dk::io::Window::wrapOutOfBoundsCursor(bool allowInnerBorder) const
{
	glm::ivec2 windowSize = property<dk::io::properties::window::size>();
	glm::ivec2 cursorPos  = cursorP();
	bool wrapped = false;

	// Horizontal
	if (cursorPos.x < allowInnerBorder && !m_context->m_cursorWrapContext.wrappedRight) {
		cursorPos.x = windowSize.x - 1 - allowInnerBorder;
		m_context->m_cursorWrapContext.wrappedLeft = true;
		wrapped = true;
	} 
	else if (cursorPos.x >= windowSize.x - allowInnerBorder && !m_context->m_cursorWrapContext.wrappedLeft) {
		cursorPos.x = allowInnerBorder;
		m_context->m_cursorWrapContext.wrappedRight = true;
		wrapped = true;
	}
	else {
		m_context->m_cursorWrapContext.wrappedLeft  = false;
		m_context->m_cursorWrapContext.wrappedRight = false;
	}

	// Vertical
	if (cursorPos.y < allowInnerBorder && !m_context->m_cursorWrapContext.wrappedBottom) {
		cursorPos.y = windowSize.y - 1 - allowInnerBorder;
		m_context->m_cursorWrapContext.wrappedTop = true;
		wrapped = true;
	} 
	else if (cursorPos.y >= windowSize.y - allowInnerBorder && !m_context->m_cursorWrapContext.wrappedTop) {
		cursorPos.y = allowInnerBorder;
		m_context->m_cursorWrapContext.wrappedBottom = true;
		wrapped = true;
	} 
	else {
		m_context->m_cursorWrapContext.wrappedTop    = false;
		m_context->m_cursorWrapContext.wrappedBottom = false;
	}

	if (wrapped)
		SDL_WarpMouseInWindow(m_context->m_window, cursorPos.x, cursorPos.y);
	return wrapped;
}

dk::io::Window::~Window()
{ }

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
	SDL_SetWindowBordered(window.m_context->m_window, isEnabled);
	spdlog::trace("Set border {} for window: {}", (isEnabled ? "enabled" : "disabled"), window.m_context->m_id);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::theme& theme)
{
	BOOL USE_DARK_MODE = theme == dk::io::properties::window::theme::dark;
	BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
		window.m_context->m_windowHandle, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
		&USE_DARK_MODE, sizeof(USE_DARK_MODE)));
	spdlog::trace("Set {} theme for window: {}", (USE_DARK_MODE ? "dark" : "light"), window.m_context->m_id);

	// hack: Have to hide and show the window to apply color change
	bool border = window.property<dk::io::properties::window::border>() == dk::io::properties::window::border::enabled;
	SDL_SetWindowBordered(window.m_context->m_window, !border);
	SDL_SetWindowBordered(window.m_context->m_window, border);
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
	SDL_SetWindowMouseGrab(window.m_context->m_window, (bool)mode);
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
	case dk::io::properties::window::mode::fullscreen: return SDL_WINDOW_FULLSCREEN;
		break;
	default:
		break;
	}
	//SDL_WINDOW_FULLSCREEN, SDL_WINDOW_FULLSCREEN_DESKTOP
}
