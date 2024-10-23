#include <unordered_map>

#include "Windows.h"
#pragma comment (lib, "Dwmapi")
#include <dwmapi.h>
#include "SDL2/SDL_syswm.h"

#include "graphics_includes.h"
#include "devkit/devkit.h"
#include "devkit/log.h"

using namespace NS_DEVKIT;

struct SDLWindowImpl {
	SDL_Window*			window = nullptr;
	Uint32				windowId = 0;

	bool				shouldQuit = false;

	SDL_GLContext		glContext = nullptr;

	Window::Properties&	properties;
	Window::Properties&	prevProperties;

	SDLWindowImpl(Window::Properties& _properties, Window::Properties& _prevProperties)
		: properties(_properties)
		, prevProperties(_prevProperties)
	{ }

	void handleEvent(SDL_WindowEvent event) 
	{
		switch (event.event)
		{
		case SDL_WINDOWEVENT_CLOSE: shouldQuit = true; DBG("Window#{} closed", windowId); break;
		case SDL_WINDOWEVENT_RESIZED: {
			properties.width  = event.data1; prevProperties.width  = event.data1;
			properties.height = event.data2; prevProperties.height = event.data2;
			DBG("Window#{} resized {} {}", windowId, properties.width, properties.height);
		} break;
		default:
			break;
		}
	}
};

struct SDL : public SingletonBase<SDL> {
	std::unordered_map<Uint32, SDLWindowImpl*> activeWindows;

	SDL() 
	{
		// Init SDL
		SDL_Init(SDL_INIT_EVERYTHING);

		// Use OpenGL 3.3
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	}

	void handleEvents() 
	{
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type)
			{
			case SDL_WINDOWEVENT: {
				auto it = activeWindows.find(event.window.windowID);
				if (it != activeWindows.end())
					it->second->handleEvent(event.window);
			} break;
			default:
				break;
			}
		}
	}
};

#define UPDATE_PROPERTY(name, function) updateProperty(&Properties::name, function, #name)

struct Window::Impl {
	Window*			m_window;
	SDLWindowImpl	m_sdlImpl;

	bool			m_shouldQuit = false;
	bool			m_isOpen = false;

	Properties		m_properties{};
	Properties		m_prevProperties{};

	HWND			m_windowsWindowHandle = nullptr;

	Impl(Window* window, Properties properties)
		: m_window(window)
		, m_sdlImpl(m_properties, m_prevProperties)
		, m_properties(properties)
		, m_prevProperties(properties)
	{ }

	WindowStatus open() 
	{
		// Create window
		m_sdlImpl.window = SDL_CreateWindow(m_properties.title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			m_properties.width, m_properties.height, SDL_WINDOW_RESIZABLE | SDL_RENDERER_ACCELERATED);

		// Add window to list of open windows
		m_sdlImpl.windowId = SDL_GetWindowID(m_sdlImpl.window);
		SDL::instance().activeWindows.insert({ m_sdlImpl.windowId, &m_sdlImpl });
		m_isOpen = true;

		// Create opengl context
		m_sdlImpl.glContext = SDL_GL_CreateContext(m_sdlImpl.window);
		if (!m_sdlImpl.glContext) {
			ERR("OpenGL context could not be created! SDL Error: {}", SDL_GetError());
			return WindowStatus::GraphicsInitError;
		}
		SDL_GL_MakeCurrent(m_sdlImpl.window, m_sdlImpl.glContext);

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

		updateProperties();
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

	void close() 
	{
		SDL::instance().activeWindows.erase(m_sdlImpl.windowId);
		SDL_DestroyWindow(m_sdlImpl.window);
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
		UPDATE_PROPERTY(vsyncEnabled,  SDL_GL_SetSwapInterval);
		UPDATE_PROPERTY(useDarkTheme,  [this] (bool v) { doUseDarkTheme(v); });
		UPDATE_PROPERTY(width,		   [this] (unsigned w) { SDL_SetWindowSize(m_sdlImpl.window, w, m_properties.height); });
		UPDATE_PROPERTY(height,		   [this] (unsigned h) { SDL_SetWindowSize(m_sdlImpl.window, m_properties.width,  h); });
		UPDATE_PROPERTY(title,		   [this] (const std::string& title) { SDL_SetWindowTitle(m_sdlImpl.window, title.c_str()); });
		UPDATE_PROPERTY(borderEnabled, [this] (bool e) { SDL_SetWindowBordered(m_sdlImpl.window, (SDL_bool)e); });

		m_prevProperties = m_properties;
	}
};

Window::Window(Properties properties)
    : impl(std::make_unique<Impl>(this, properties))
{ }

Window::WindowStatus Window::open() 
{
	return impl->open();
}

bool Window::isOpen() 
{
	return !impl->m_sdlImpl.shouldQuit;
}

void Window::render() 
{
	SDL::instance().handleEvents();
	impl->updateProperties();
}

Window::Properties& Window::properties() 
{
	return impl->m_properties;
}

Window::~Window() { }
