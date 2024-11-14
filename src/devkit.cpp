#include <unordered_map>

#include "Windows.h"
#pragma comment (lib, "Dwmapi")
#include <dwmapi.h>
#include "SDL2/SDL_syswm.h"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include "graphics_includes.h"
#include "devkit/devkit.h"
#include "devkit/log.h"

#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace NS_DEVKIT;

struct OpenGLViewportImpl { 
	glm::i32vec2 offset; 
	glm::i32vec2 size;

	inline static std::vector<OpenGLViewportImpl> s_viewportStack{};
};

void pushViewport(glm::i32vec2 offset, glm::i32vec2 size)
{
	glViewport(offset.x, offset.y, size.x, size.y);
	OpenGLViewportImpl::s_viewportStack.push_back({ offset, size });
}

void popViewport()
{
	OpenGLViewportImpl::s_viewportStack.pop_back();
	if (OpenGLViewportImpl::s_viewportStack.empty())
		return;
	auto viewport = OpenGLViewportImpl::s_viewportStack.back();
	glViewport(viewport.offset.x, viewport.offset.y, viewport.size.x, viewport.size.y);
}

struct SDLWindowImpl {
	SDL_Window*			window = nullptr;
	Uint32				windowId = 0;

	bool				isOpen = false;

	SDL_GLContext		glContext = nullptr;

	Window::Properties&	properties;
	Window::Properties&	prevProperties;

	// For keeping track of whether to handle events on beginFrame
	unsigned long long  handledTick = 0;

	SDLWindowImpl(Window::Properties& _properties, Window::Properties& _prevProperties)
		: properties(_properties)
		, prevProperties(_prevProperties)
	{ }

	void handleEvent(SDL_WindowEvent event);

	void close();
};

struct SDLMouseState {
	Uint8 buttons	  = 0;
	Uint8 prevButtons = 0;
};

struct SDL : public SingletonBase<SDL> {
	std::unordered_map<Uint32, SDLWindowImpl*>	activeWindows;
	SDLMouseState								mouseState;

	std::unordered_map<SDL_Window*, Window*>	sdlToWindow;
	SDL_Window*									focusedSdlWindow = nullptr;
	SDL_GLContext								currentGlContext = nullptr;

	unsigned long long							tickCount = 1;

	SDL() 
	{
		// Init SDL
		SDL_Init(SDL_INIT_EVERYTHING);

		// Use OpenGL 3.3
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		DBG("Initialized SDL");
	}

	void handleEvents() 
	{
		// Update mouse state
		mouseState.prevButtons = mouseState.buttons;
		mouseState.buttons = SDL_GetMouseState(nullptr, nullptr);

		// Poll events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type)
			{
			case SDL_WINDOWEVENT: {
				// Pass event to affected window
				auto it = activeWindows.find(event.window.windowID);
				if (it != activeWindows.end())
					it->second->handleEvent(event.window);
			} break;
			default:
				break;
			}

			// Pass event to ImGui
			if (focusedSdlWindow) {
				auto focusedWindowIt = sdlToWindow.find(focusedSdlWindow);
				if (focusedWindowIt != sdlToWindow.end())
					ImGui::SetCurrentContext(sdlToWindow.at(focusedSdlWindow)->getImGuiContext());
			}
			ImGui_ImplSDL2_ProcessEvent(&event);
		}
	}
};

SDL_GLContext currentGlContext() {
	return SDL::instance().currentGlContext;
}

void SDLWindowImpl::handleEvent(SDL_WindowEvent event)
{
	auto flags = SDL_GetWindowFlags(window);

	// Check if window has focus
	if (flags & SDL_WINDOW_INPUT_FOCUS)
		SDL::instance().focusedSdlWindow = window;

	switch (event.event)
	{
	case SDL_WINDOWEVENT_CLOSE: close(); DBG("Window#{} closed", windowId); break;
	case SDL_WINDOWEVENT_RESIZED: {
		properties.width = event.data1; prevProperties.width = event.data1;
		properties.height = event.data2; prevProperties.height = event.data2;
		DBG("Window#{} resized {} {}", windowId, properties.width, properties.height);
	} break;
	default:
		break;
	}
}

void SDLWindowImpl::close()
{
	if (!isOpen)
		return;
	SDL::instance().activeWindows.erase(windowId);
	SDL::instance().sdlToWindow.erase(window);
	SDL_DestroyWindow(window);
	isOpen = false;
}

#define UPDATE_PROPERTY(name, function) updateProperty(&Properties::name, function, #name)

struct Window::Impl {
	Window*			m_window;
	SDLWindowImpl	m_sdlImpl;

	Properties		m_properties{};
	Properties		m_prevProperties{};

	HWND			m_windowsWindowHandle = nullptr;

	ImGuiContext*   m_imguiContext = nullptr;

	Impl(Window* window, Properties properties)
		: m_window(window)
		, m_sdlImpl(m_properties, m_prevProperties)
		, m_properties(properties)
		, m_prevProperties(properties)
	{ }

	WindowStatus open() 
	{
		if (m_sdlImpl.isOpen)
			return WindowStatus::WindowAllreadyOpen;
		
		// Create window
		m_sdlImpl.window = SDL_CreateWindow(m_properties.title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			m_properties.width, m_properties.height, SDL_WINDOW_RESIZABLE | SDL_RENDERER_ACCELERATED);
		m_sdlImpl.isOpen = true;

		// Add window to list of open windows
		m_sdlImpl.windowId = SDL_GetWindowID(m_sdlImpl.window);
		SDL::instance().activeWindows.insert({ m_sdlImpl.windowId, &m_sdlImpl });
		SDL::instance().sdlToWindow.insert({ m_sdlImpl.window, m_window });

		// Create opengl context
		m_sdlImpl.glContext = SDL_GL_CreateContext(m_sdlImpl.window);
		if (!m_sdlImpl.glContext) {
			ERR("OpenGL context could not be created! SDL Error: {}", SDL_GetError());
			return WindowStatus::GraphicsInitError;
		}
		SDL_GL_MakeCurrent(m_sdlImpl.window, m_sdlImpl.glContext);
		SDL::instance().currentGlContext = m_sdlImpl.glContext;

		// Initialize GLEW after creating OpenGL context
		glewExperimental = GL_TRUE;
		GLenum glewError = glewInit();
		if (glewError != GLEW_OK) {
			ERR("Error initializing GLEW!");
			return WindowStatus::GraphicsInitError;
		}
		// Remove error caused by glewExperimental
		glGetError();

		// Get window handle
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(m_sdlImpl.window, &wmInfo);
		m_windowsWindowHandle = wmInfo.info.win.window;

		initImGui();

		updateProperties();

		DBG("Window#{} opened", m_sdlImpl.windowId);
		return WindowStatus::Ok;
	}

	void initImGui() {
		m_imguiContext = ImGui::CreateContext();
		ImGui::SetCurrentContext(m_imguiContext);

		ImGui_ImplSDL2_InitForOpenGL(m_sdlImpl.window, m_sdlImpl.glContext);
		ImGui_ImplOpenGL3_Init("#version 330");

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImGui::StyleColorsClassic();

		// TODO: https://github.com/ocornut/imgui/issues/4952
	}

	template <typename T>
	bool propertyChanged(T Properties::*property)
	{
		return m_properties.*property != m_prevProperties.*property;
	}

	template <typename T, typename Func>
	void updateProperty(T Properties::* property, Func func, const char* name)
	{
		if (!propertyChanged(property))
			return;
		DBG("Window#{} properties.{} = {}", m_sdlImpl.windowId, name, m_properties.*property);
		func(m_properties.*property);
	}

	void doUseDarkTheme(bool useDarkTheme) 
	{
		BOOL USE_DARK_MODE = (bool)useDarkTheme;
		BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
			m_windowsWindowHandle, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
			&USE_DARK_MODE, sizeof(USE_DARK_MODE)));

		ShowWindow(m_windowsWindowHandle, SW_MINIMIZE);
		ShowWindow(m_windowsWindowHandle, SW_RESTORE);
	}

	void updateProperties()
	{
		UPDATE_PROPERTY(vsyncEnabled,    SDL_GL_SetSwapInterval);
		UPDATE_PROPERTY(useDarkTheme,    [this] (bool v) { doUseDarkTheme(v); });
		UPDATE_PROPERTY(width,		     [this] (unsigned w) { SDL_SetWindowSize(m_sdlImpl.window, w, m_properties.height); });
		UPDATE_PROPERTY(height,		     [this] (unsigned h) { SDL_SetWindowSize(m_sdlImpl.window, m_properties.width,  h); });
		UPDATE_PROPERTY(title,		     [this] (const std::string& title) { SDL_SetWindowTitle(m_sdlImpl.window, title.c_str()); });
		UPDATE_PROPERTY(borderEnabled,   [this] (bool e) { SDL_SetWindowBordered(m_sdlImpl.window, (SDL_bool)e); });
		UPDATE_PROPERTY(backgroundColor, [](Color){  });
		UPDATE_PROPERTY(alwaysOnTop,     [this] (bool e) { SDL_SetWindowAlwaysOnTop(m_sdlImpl.window, (SDL_bool)e); });
		SDL_SetWindowAlwaysOnTop(m_sdlImpl.window, (SDL_bool)m_properties.alwaysOnTop);

		m_prevProperties = m_properties;
	}

	// Handles events if this is the first window which begins a frame in this tick
	void tryHandleEvents() {
		if (m_sdlImpl.handledTick == SDL::instance().tickCount - 1) {
			++m_sdlImpl.handledTick;
			++SDL::instance().tickCount;
			SDL::instance().handleEvents();
		}
		else {
			m_sdlImpl.handledTick = SDL::instance().tickCount - 1;
			return;
		}
	}
};

Window::Window()
	: impl(std::make_unique<Impl>(this, Window::Properties{}))
{ }

Window::Window(Properties properties)
	: impl(std::make_unique<Impl>(this, properties))
{ }

Window::WindowStatus Window::open() 
{
	return impl->open();
}

void Window::close()
{
	impl->m_sdlImpl.close();
}

bool Window::isOpen() const 
{
	return impl->m_sdlImpl.isOpen;
}

bool Window::isAnyOpen() {
	return SDL::instance().activeWindows.size();
}

ImGuiContext* Window::getImGuiContext() const {
	return impl->m_imguiContext;
}

Window::Properties& Window::properties() 
{
	return impl->m_properties;
}

Window::~Window() { close(); }

bool Window::beginFrame()
{
	if (!isOpen())
		return false;

	impl->updateProperties();
	SDL_GL_MakeCurrent(impl->m_sdlImpl.window, impl->m_sdlImpl.glContext);
	SDL::instance().currentGlContext = impl->m_sdlImpl.glContext;

	// Set viewport
	pushViewport({ 0, 0 }, { impl->m_properties.width, impl->m_properties.height });

	// Start the Dear ImGui frame
	ImGui::SetCurrentContext(impl->m_imguiContext);
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	//ImGui::DockSpaceOverViewport((const ImGuiViewport*)0, ImGuiDockNodeFlags_PassthruCentralNode);

	return true;
}

void Window::endFrame()
{
	if (!isOpen())
		return;

	// Render ImGui draw data
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	ImGui::EndFrame();

	// Swap buffers
	SDL_GL_SwapWindow(impl->m_sdlImpl.window);

	// Clear buffer
	const Color& bckgColor = properties().backgroundColor;
	glClearColor(bckgColor.r / 255.f, bckgColor.g / 255.f, bckgColor.b / 255.f, bckgColor.a / 255.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	popViewport();

	// Handle events
	impl->tryHandleEvents();
	if (!isOpen())
		return;
}

unsigned long long Window::tickCount() {
	return SDL::instance().tickCount;
}

Window& Window::focused()
{
	return *SDL::instance().sdlToWindow.at(SDL::instance().focusedSdlWindow);
}

bool Mouse::clicked(Mouse::Button button) 
{
	return isButtonDown(button) && !(SDL::instance().mouseState.prevButtons & button);
}

bool Mouse::isButtonDown(Mouse::Button button) 
{
	return (SDL::instance().mouseState.buttons & button);
}

bool NS_DEVKIT::ImGuiAnyHovered() {
	bool anyHovered = false;
	auto currentContext = ImGui::GetCurrentContext();
	for (auto& sdlWindowImpl : SDL::instance().activeWindows) {
		const auto& window = SDL::instance().sdlToWindow.at(sdlWindowImpl.second->window);

		ImGui::SetCurrentContext(window->getImGuiContext());
		anyHovered |= ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered();
	}
	ImGui::SetCurrentContext(currentContext);
	return anyHovered;
}
