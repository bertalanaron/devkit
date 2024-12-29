#include <unordered_map>

#include "Windows.h"
#pragma comment (lib, "Dwmapi")
#include <dwmapi.h>
#undef DELETE

#include "SDL2/SDL_syswm.h"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include "graphics_includes.h"
#include "devkit/devkit.h"

#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "devkit/graphics.h"

using namespace NS_DEVKIT;

struct OpenGLViewportSizeImpl : glm::i32vec2 {
	using glm::i32vec2::i32vec2;

	inline static std::vector<OpenGLViewportSizeImpl> s_stack{};
};

struct OpenGLViewportOffsetImpl : glm::i32vec2 {
	using glm::i32vec2::i32vec2;

	inline static std::vector<OpenGLViewportOffsetImpl> s_stack{};
};

void pushViewportSize(glm::i32vec2 size)
{
	const auto& offset = currentViewportOffset().value_or(glm::i32vec2{ 0, 0 });
	glViewport(offset.x, offset.y, size.x, size.y);
	OpenGLViewportSizeImpl::s_stack.push_back(OpenGLViewportSizeImpl(size.x, size.y));
}

void popViewportSize()
{
	OpenGLViewportSizeImpl::s_stack.pop_back();
	if (OpenGLViewportSizeImpl::s_stack.empty())
		return;
	const auto& size = OpenGLViewportSizeImpl::s_stack.back();
	const auto& offset = currentViewportOffset().value_or(glm::i32vec2{ 0, 0 });
	glViewport(offset.x, offset.y, size.x, size.y);
}

void pushViewportOffset(glm::i32vec2 offset)
{
	const auto& size = currentViewportSize().value_or(glm::i32vec2{ 0, 0 });	
	glViewport(offset.x, offset.y, size.x, size.y);
	OpenGLViewportOffsetImpl::s_stack.push_back(OpenGLViewportOffsetImpl(offset.x, offset.y));
}

void popViewportOffset()
{
	OpenGLViewportOffsetImpl::s_stack.pop_back();
	if (OpenGLViewportOffsetImpl::s_stack.empty())
		return;
	const auto& size = OpenGLViewportSizeImpl::s_stack.back();
	const auto& offset = OpenGLViewportOffsetImpl::s_stack.back();
	glViewport(offset.x, offset.y, size.x, size.y);
}

std::optional<glm::i32vec2> currentViewportSize()
{
	if (OpenGLViewportSizeImpl::s_stack.empty())
		return std::nullopt;
	return OpenGLViewportSizeImpl::s_stack.back();
}

std::optional<glm::i32vec2> currentViewportOffset()
{
	if (OpenGLViewportOffsetImpl::s_stack.empty())
		return std::nullopt;
	return OpenGLViewportOffsetImpl::s_stack.back();
}

struct SDLGlobalMouseState {
	Uint8        buttons	 = 0;
	Uint8        prevButtons = 0;
	float		 wheel = 0.f;
	float		 prevWheel = 0.f;
	glm::i32vec2 globalPosition;
};

struct SDLWindowMouseState {
	glm::i32vec2                     windowPosition;
	std::array<Cursor::DragStart, 3> buttonToDragstart;
};

struct SDLWindowImpl {
	SDL_Window*			window = nullptr;
	Uint32				windowId = 0;

	bool				isOpen = false;
	SDLWindowMouseState mouseState;

	SDL_GLContext		glContext = nullptr;

	Window::Properties&	properties;
	Window::Properties&	prevProperties;


	// For keeping track of whether to handle events on beginFrame
	unsigned long long  handledTick = 0;

	SDLWindowImpl(Window::Properties& _properties, Window::Properties& _prevProperties)
		: properties(_properties)
		, prevProperties(_prevProperties)
	{ }

	void updateState();
	void handleEvent(SDL_WindowEvent event);

	void close();
};

struct KeyboardState {
	int			 arraySize = 0;
	const Uint8* data = nullptr;
	Uint8*		 prevData = nullptr;
	SDL_Keymod	 mod;
	SDL_Keymod	 prevMod;

	void update() {
		memcpy(prevData, data, arraySize);
	}

	void init() {
		data = SDL_GetKeyboardState(&arraySize);
		prevData = new Uint8[arraySize];
	}

	void setMod(SDL_Keymod _mod) {
		prevMod = mod;
		mod = _mod;
	}
};

struct SDL : public SingletonBase<SDL> {
	std::unordered_map<Uint32, SDLWindowImpl*>	activeWindows;
	SDLGlobalMouseState							mouseState;
	KeyboardState							    keyboardState{};

	std::unordered_map<SDL_Window*, Window*>	sdlToWindow;
	std::unordered_map<void*, SDLWindowImpl*>	glToWindow;
	SDL_Window*									focusedSdlWindow = nullptr;
	SDL_GLContext								currentGlContext = nullptr;
	SDL_Window*								    currentWindow = nullptr;

	unsigned long long							tickCount = 1;

	SDL() 
	{
		// Init SDL
		SDL_Init(SDL_INIT_EVERYTHING);

		// Use OpenGL 3.3
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		keyboardState.init();

		spdlog::info("Initialized SDL");
	}

	void handleEvents() 
	{
		// Update mouse state
		mouseState.prevButtons = mouseState.buttons;
		mouseState.buttons = SDL_GetGlobalMouseState(&mouseState.globalPosition.x, &mouseState.globalPosition.y);
		mouseState.prevWheel = mouseState.wheel;  
		mouseState.wheel = 0.f;  // Wheel state is updated from events

		// Update keyboard state
		keyboardState.update();
		keyboardState.setMod(SDL_GetModState());

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
			case SDL_MOUSEWHEEL: {
				SDL_MouseWheelEvent wheelEvent = event.wheel;
				mouseState.wheel = wheelEvent.preciseY;
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

void SDLWindowImpl::updateState()
{
	// Update mouse state
	int y;
	SDL_GetMouseState(&mouseState.windowPosition.x, &y);
	mouseState.windowPosition.y = properties.height - y;
	// Update drag start positions
	for (int i = 0; i < 3; ++i) {
		Mouse::Button button = Mouse::Button(1 << i);
		if (Mouse::clicked(button))
			mouseState.buttonToDragstart.at(i) = { 
				.mod = (KeyMod)SDL::instance().keyboardState.mod, 
				.position = mouseState.windowPosition };
	}
}

void SDLWindowImpl::handleEvent(SDL_WindowEvent event)
{
	auto flags = SDL_GetWindowFlags(window);

	// Check if window has focus
	if (flags & SDL_WINDOW_INPUT_FOCUS)
		SDL::instance().focusedSdlWindow = window;

	switch (event.event)
	{
	case SDL_WINDOWEVENT_CLOSE: close(); spdlog::info("Window#{} closed", windowId); break;
	case SDL_WINDOWEVENT_RESIZED: {
		properties.width = event.data1; prevProperties.width = event.data1;
		properties.height = event.data2; prevProperties.height = event.data2;
		spdlog::info("Window#{} resized {} {}", windowId, properties.width, properties.height);
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
	SDL::instance().glToWindow.erase(glContext);
	SDL_DestroyWindow(window);
	isOpen = false;
}

#define UPDATE_PROPERTY(name, function) updateProperty(&Properties::name, function, #name)

struct Window::Impl {
	Window*			m_window;
	SDLWindowImpl	m_sdlImpl;

	Properties		m_properties{};
	Properties		m_prevProperties{};
	Cursor			m_cursor;

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
			spdlog::error("OpenGL context could not be created! SDL Error: {}", SDL_GetError());
			return WindowStatus::GraphicsInitError;
		}
		SDL_GL_MakeCurrent(m_sdlImpl.window, m_sdlImpl.glContext);
		SDL::instance().currentGlContext = m_sdlImpl.glContext;
		SDL::instance().currentWindow = m_sdlImpl.window;
		SDL::instance().glToWindow.insert({ m_sdlImpl.glContext, &m_sdlImpl });

		// TODO: do this as part of frame producer properties
		// Enable depth testing
		glEnable(GL_DEPTH_TEST);  

		// Initialize GLEW after creating OpenGL context
		glewExperimental = GL_TRUE;
		GLenum glewError = glewInit();
		if (glewError != GLEW_OK) {
			spdlog::error("Error initializing GLEW!");
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

		spdlog::info("Window#{} opened", m_sdlImpl.windowId);
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
		spdlog::info("Window#{} properties.{} = {}", m_sdlImpl.windowId, name, m_properties.*property);
		func(m_properties.*property);
	}

	void doUseDarkTheme(bool useDarkTheme) 
	{
		BOOL USE_DARK_MODE = (bool)useDarkTheme;
		BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
			m_windowsWindowHandle, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
			&USE_DARK_MODE, sizeof(USE_DARK_MODE)));

		bool border = m_properties.borderEnabled;
		SDL_SetWindowBordered(m_sdlImpl.window, (SDL_bool)!border);
		SDL_SetWindowBordered(m_sdlImpl.window, (SDL_bool)border);
	}

	void updateProperties()
	{
		UPDATE_PROPERTY(vsyncEnabled,    SDL_GL_SetSwapInterval);
		UPDATE_PROPERTY(useDarkTheme,    [this] (bool v) { doUseDarkTheme(v); });
		UPDATE_PROPERTY(width,		     [this] (unsigned w) { SDL_SetWindowSize(m_sdlImpl.window, w, m_properties.height); });
		UPDATE_PROPERTY(height,		     [this] (unsigned h) { SDL_SetWindowSize(m_sdlImpl.window, m_properties.width,  h); });
		UPDATE_PROPERTY(title,		     [this] (const std::string& title) { SDL_SetWindowTitle(m_sdlImpl.window, title.c_str()); });
		UPDATE_PROPERTY(borderEnabled,   [this] (bool e) { SDL_SetWindowBordered(m_sdlImpl.window, (SDL_bool)e); });
		UPDATE_PROPERTY(backgroundColor, [](glm::vec4){  });
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

const Window::Properties& Window::properties() const
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
	SDL::instance().currentWindow = impl->m_sdlImpl.window;

	// Set viewport
	pushViewportSize({ impl->m_properties.width, impl->m_properties.height });

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
	const glm::vec4& bckgColor = properties().backgroundColor;
	glClearColor(bckgColor.r, bckgColor.g, bckgColor.b, bckgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	popViewportSize();

	// Handle events
	impl->tryHandleEvents();
	impl->m_sdlImpl.updateState(); // has to be after tryHandleEvents
	impl->m_cursor = Cursor(
		{ 0, 0 }, 
		{ impl->m_properties.width, impl->m_properties.height }, 
		impl->m_sdlImpl.mouseState.buttonToDragstart, 
		impl->m_sdlImpl.mouseState.windowPosition);
	if (!isOpen())
		return;
}

glm::u32vec2 Window::frameSize() const
{
	return { properties().width, properties().height };
}

void* Window::context()
{
	return impl->m_sdlImpl.glContext;
}

const Cursor& Window::cursor() const 
{
	return impl->m_cursor;
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

//float Mouse::wheelStart(WheelDirection direction)
//{
//	return (SDL::instance().mouseState.wheel == SDL::instance().mouseState.prevWheel)
//		? WheelDirection::None
//		: (WheelDirection)SDL::instance().mouseState.wheel;
//}

float Mouse::wheel(WheelDirection direction) 
{
	float wheel = SDL::instance().mouseState.wheel;
	switch (direction)
	{
	case dk::Mouse::WheelDirection::None: return (fabs(wheel) > 0) ? 0.f : 1.f;
	case dk::Mouse::WheelDirection::Up: return (wheel > 0) ? wheel : 0.f;
	case dk::Mouse::WheelDirection::Down: return (wheel < 0) ? wheel : 0.f;
	case dk::Mouse::WheelDirection::Any: return wheel;
	default:
		throw 0.f;
	}
}

bool NS_DEVKIT::ImGuiAnyHovered() 
{
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

bool Keyboard::isKeyDown(Key key)
{
	return SDL::instance().keyboardState.data[(uint8_t)key];
}

bool Keyboard::isKeyDown(KeyMod mod)
{
	return checkMods((KeyMod)SDL::instance().keyboardState.mod, mod);
}

bool Keyboard::keyPressed(Key key)
{
	return SDL::instance().keyboardState.data[(uint8_t)key] && !SDL::instance().keyboardState.prevData[(uint8_t)key];
}

bool Keyboard::keyPressed(KeyMod mod)
{
	return (SDL::instance().keyboardState.mod & (uint8_t)mod) && !(SDL::instance().keyboardState.prevMod & (uint8_t)mod);
}

bool Keyboard::checkMods(KeyMod a, KeyMod b) 
{
	if (a == KeyMod::NONE && b == KeyMod::NONE)
		return true;
	return (int)a & (int)b;
}

InputCombination::InputCombination(KeyMod mod, Activator activator)
	: m_mod(mod), m_activator(activator)
{ }

InputCombination::InputCombination(Activator activator)
	: m_mod(std::nullopt), m_activator(activator)
{ }

bool InputCombination::active() const
{
	if (m_mod.has_value() && !Keyboard::isKeyDown(m_mod.value()))
		return false;

	return std::visit(overload{
		[](const Key& key) -> bool {
			// m_activator: Key
			return Keyboard::isKeyDown(key); },
		[](const Mouse::Button& button) -> bool { 
			// m_activator: Mouse::Button
			return Mouse::isButtonDown(button); },
		[](const Mouse::WheelDirection& wheel) -> bool {
			// m_activator: Mouse::WheelDirection
			return Mouse::wheel(wheel); },
		[target = m_target](const Cursor::Region& region) -> bool {
			// m_activator: Cursor::Region
			return std::get<std::reference_wrapper<const Cursor>>(target).get().insideRegion(region); }
		}, m_activator);
}

bool InputCombination::activated() const
{
	if (m_mod.has_value() && !Keyboard::isKeyDown(m_mod.value()))
		return false;

	return std::visit(overload{
		[](const Key& key) -> bool {
			// m_activator: Key
			return Keyboard::keyPressed(key); },
		[](const Mouse::Button& button) -> bool { 
			// m_activator: Mouse::Button
			return Mouse::clicked(button); },
		[](const Mouse::WheelDirection& wheel) -> bool {
			// m_activator: Mouse::WheelDirection
			return Mouse::wheel(wheel); },
		[target = m_target](const Cursor::Region& region) -> bool {
			// m_activator: Cursor::Region
			return std::get<std::reference_wrapper<const Cursor>>(target).get().enteredRegion(region); }
		}, m_activator);
}

struct Viewport::Impl {
	IFrameProducer& m_parent;
	Properties		m_properties;
	Cursor			m_cursor;

	Impl(IFrameProducer& parent, Properties properties)
		: m_parent(parent)
		, m_properties(properties)
	{ }
};

Viewport::Viewport(IFrameProducer& parent)
	: m_impl(std::make_unique<Impl>(parent, Properties()))
{ }

Viewport::Viewport(IFrameProducer& parent, Properties properties)
	: m_impl(std::make_unique<Impl>(parent, properties))
{ }

Viewport::Properties& Viewport::properties()
{
	return m_impl->m_properties;
}

const Cursor& Viewport::cursor() const
{
	return m_impl->m_cursor;
}

Viewport::~Viewport() { }

bool Viewport::beginFrame()
{
	// Begin parent frame
	m_impl->m_parent.beginFrame();

	// Set viewport size and offset
	glm::u32vec2 size(  std::min(m_impl->m_parent.frameSize().x, m_impl->m_properties.size.x),
		std::min(m_impl->m_parent.frameSize().y, m_impl->m_properties.size.y));
	pushViewportSize(size);
	glm::i32vec2 offset(std::max(0, m_impl->m_properties.offset.x),
		std::max(0, m_impl->m_properties.offset.y));
	pushViewportOffset(offset);

	// Clear buffer
	if (m_impl->m_properties.doClear) {
		const glm::vec4& clearColor = m_impl->m_properties.clearColor;
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	return true;
}

void Viewport::endFrame()
{
	popViewportOffset();
	popViewportSize();

	m_impl->m_parent.endFrame();

	// Update cursor
	m_impl->m_cursor = Cursor(
		properties().offset,
		properties().size,
		SDL::instance().glToWindow.at(context())->mouseState.buttonToDragstart, 
		SDL::instance().glToWindow.at(context())->mouseState.windowPosition);
}

glm::u32vec2 Viewport::frameSize() const
{
	return m_impl->m_properties.size;
}

void* Viewport::context()
{
	return m_impl->m_parent.context();
}

Cursor::Cursor(glm::i32vec2 offset, glm::u32vec2 size, std::array<DragStart, 3> dragStarts, glm::vec2 windowPosition)
	: m_viewportOffset(offset)
	, m_viewportSize(size)
	, m_dragStarts(dragStarts)
	, m_windowPosition(windowPosition)
	, m_prevWindowPosition(windowPosition)
{ }

Cursor& Cursor::operator=(const Cursor & other)
{
	m_prevWindowPosition = m_windowPosition;

	m_viewportOffset = other.m_viewportOffset;
	m_viewportSize   = other.m_viewportSize;
	m_dragStarts	 = other.m_dragStarts;
	m_windowPosition = other.m_windowPosition;

	return *this;
}

glm::vec2 Cursor::pointToNdc(const glm::vec2 point) const
{
	return (point - (glm::vec2)m_viewportOffset) / (glm::vec2)m_viewportSize;
}

glm::vec2 Cursor::vecToNdc(const glm::vec2 vec) const
{
	return vec / (glm::vec2)m_viewportSize;
}

glm::vec2 Cursor::position(CoordinateSystem coordSys) const
{
	switch (coordSys)
	{
	case dk::Cursor::Ndc:
		return pointToNdc(m_windowPosition);
	case dk::Cursor::Window:
		return m_windowPosition;
	case dk::Cursor::Global: 
		return SDL::instance().mouseState.globalPosition;
	default:
		return {};
	}
}

glm::vec2 Cursor::drag(Mouse::Button button, std::optional<KeyMod> oMod, CoordinateSystem coordSys) const
{
	if (button == Mouse::Button::Any)
		throw std::runtime_error("Button has to be specific.");
	
	// Return if not dragging
	if (!Mouse::isButtonDown(button))
		return {};

	// Get drag of specified button
	int buttonIndex;
	switch (button)
	{
	case dk::Mouse::Left: buttonIndex = 0; break;
	case dk::Mouse::Middle: buttonIndex = 1; break;
	case dk::Mouse::Right: buttonIndex = 2; break;
	default:
		return {};
	}
	const auto& dragInstance = m_dragStarts.at(buttonIndex);

	// Return if mods don't match specified
	if (oMod.has_value() && !Keyboard::checkMods(dragInstance.mod, oMod.value()))
		return {};

	// Return drag in the specified coordinate system
	switch (coordSys)
	{
	case dk::Cursor::Ndc: 
		return vecToNdc(m_windowPosition - (glm::vec2)dragInstance.position);
	case dk::Cursor::Window: 
		return m_windowPosition - (glm::vec2)dragInstance.position;
	case dk::Cursor::Global:
		// TODO:
		throw std::runtime_error("Not implemented yet.");
	default:
		return {};
	}
}

glm::vec2 Cursor::delta(CoordinateSystem coordSys) const
{
	switch (coordSys)
	{
	case dk::Cursor::Ndc:
		return vecToNdc(m_windowPosition - m_prevWindowPosition);
	case dk::Cursor::Window:
		return m_windowPosition - m_prevWindowPosition;
	case dk::Cursor::Global:
		// TODO:
		throw std::runtime_error("Not implemented yet.");
	default:
		return {};
	}
}

bool Cursor::insideRegion(Region region) const
{
	// TODO:
	return true;
}

bool Cursor::enteredRegion(Region region) const
{
	// TODO: 
	return false;
}
