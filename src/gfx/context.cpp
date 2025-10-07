#include "context.h"

#include <imgui_internal.h>

const dk::io::GlobalState::State& dk::io::GlobalState::state()
{
	return instance().m_state;
}

const dk::io::GlobalState::HardwareVariantState& dk::io::GlobalState::hardware()
{
	tryInitialize();
	return instance().m_harwareVariantState;
}

dk::io::GlobalState::State& dk::io::GlobalState::mutState()
{
	return instance().m_state;
}

dk::io::GlobalState::HardwareVariantState& dk::io::GlobalState::mutHardware()
{
	return instance().m_harwareVariantState;
}

void dk::io::GlobalState::tryInitialize()
{
	if (state().initialized)
		return;

	// Init SDL
	SDL_Init(SDL_INIT_VIDEO);
	spdlog::trace("[io] SDL3 initialized");

	// Initialize OpenGL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, state().glVersionMajor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, state().glVersionMinor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK , SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS        , SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE           , state().glDepthSize);
	spdlog::trace("[io] Using OpenGL {}.{}. ", state().glVersionMajor, state().glVersionMinor);

	// Get hardware info
	// Setup dummy context
	SDL_Window* dummyWin = SDL_CreateWindow("Dummy",1,1,SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
	if (!dummyWin)
		std::terminate();
	SDL_GLContext dummyCtx = SDL_GL_CreateContext(dummyWin);
	if (!dummyCtx)
	{
		SDL_DestroyWindow(dummyWin);
		std::terminate();
	}
	SDL_GL_MakeCurrent(dummyWin, dummyCtx);
	// Get info
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &mutHardware().glMaxTextureImageUnits);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE       , &mutHardware().glMaxTextureSize);
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS  , &mutHardware().glMaxColorAttachments);
	// Destroy dummy context
	SDL_GL_DestroyContext(dummyCtx);
	SDL_DestroyWindow(dummyWin);

	mutState().initialized = true;
}

std::unique_ptr<const dk::io::WindowContext> dk::io::GlobalState::createWindowContext(
	int msaa, 
	const glm::ivec2& size)
{
	tryInitialize();

	auto ctx = std::unique_ptr<WindowContext>(new WindowContext());
	instance().m_activeContexts.insert(ctx.get());

	// Set up MSAA
	if (msaa > 1) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
	} else
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);

	// Setup SDL state
	ctx->sdlWindowContext = SDL_CreateWindow("", size.x, size.y, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	ctx->sdlWindowID = SDL_GetWindowID(ctx->sdlWindowContext);
	spdlog::trace("[io] Created window: {} with MultiSampleAntiAliasing: {}", 
		(int)ctx->sdlWindowID, msaa);

	// Create opengl context
	ctx->sdlGlContext = SDL_GL_CreateContext(ctx->sdlWindowContext);
	if (!ctx->sdlGlContext) 
	{
		spdlog::error("[io] OpenGL context could not be created! SDL Error: {}", 
			SDL_GetError());
		std::terminate();
	}
	SDL_GL_MakeCurrent(ctx->sdlWindowContext, ctx->sdlGlContext);
	spdlog::trace("[io] Created OpenGL context for window: {}", (int)ctx->sdlWindowID);

	// Initialize GLEW after creating OpenGL context
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) 
	{
		spdlog::error("[io] Error initializing GLEW!");
		std::terminate();
	}
	// Remove error caused by glewExperimental
	glGetError();
	spdlog::trace("[io] Initialized GLEW for window: {}", (int)ctx->sdlWindowID);

	// Get native window handle
#if defined(SDL_PLATFORM_WIN32)
	ctx->nativeWindowHandle = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(ctx->sdlWindowContext), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif

	// Setup ImGui context
	ctx->imguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(ctx->imguiContext);
	// Use OpenGL
	ImGui_ImplSDL3_InitForOpenGL(ctx->sdlWindowContext, ctx->sdlGlContext);
	const auto glVersionString = std::format("#version {}{}0", state().glVersionMajor, state().glVersionMinor);
	ImGui_ImplOpenGL3_Init(glVersionString.c_str());
	// Get io
	ctx->imguiIO = &ImGui::GetIO();
	ctx->imguiIO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ctx->imguiIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	// Setup dockspace
	const auto imguiDockspaceIDString = std::format("central_dockspace{}", ctx->sdlWindowID);
	ctx->imguiDockspaceId = ImHashStr(imguiDockspaceIDString.c_str());

	WindowContext::s_currentContext = ctx.get();
	return std::move(ctx);
}

void dk::io::GlobalState::removeWindowContext(const WindowContext* context)
{
	instance().m_activeContexts.erase(context);
}

const dk::io::WindowContext* dk::io::GlobalState::currentWindowContext()
{
	return WindowContext::s_currentContext;
}

dk::io::InputState dk::io::GlobalState::getInputStateOfWindow(const WindowContext* context)
{
	// TODO: implement for multiple windows
	return state().inputState;
}

void dk::io::GlobalState::showDebugWindow()
{
	if (ImGui::Begin("io::GlobalState"))
	{
		const auto ticks = std::format("ticks: {}", state().ticks);
		ImGui::Text(ticks.c_str());
		const auto cursor = std::format("cursor: ({},{})", state().cursor.x, state().cursor.y);
		ImGui::Text(cursor.c_str());
		const auto cursorWarped = std::format("cursorWarped: {}", state().cursorWarped);
		ImGui::Text(cursorWarped.c_str());
		const auto iButtons = std::format("inputState.buttons: {}", state().inputState.buttons);
		ImGui::Text(iButtons.c_str());
		const auto iKeys = std::format("inputState.keys: {}", state().inputState.keys);
		ImGui::Text(iKeys.c_str());
		const auto iModkeys = std::format("inputState.modkeys: {}", state().inputState.modkeys);
		ImGui::Text(iModkeys.c_str());
		const auto iWheel = std::format("inputState.wheel: {}", state().inputState.wheel);
		ImGui::Text(iWheel.c_str());
		const auto iWheelDirection = std::format("inputState.wheelDirection: {}", state().inputState.wheelDirection);
		ImGui::Text(iWheelDirection.c_str());
	}
	ImGui::End();
}

void dk::io::GlobalState::pollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) 
	{
		const bool isWindowEvent = event.type >= SDL_EVENT_WINDOW_FIRST 
			&& event.type <= SDL_EVENT_WINDOW_LAST;

		// Handle global events and dispatch window events to the handlers
		// of their corresponding windows
		if (isWindowEvent)
			dispatchEvent(event.window);
		else
			handleGlobalEvent(event);

		// Pass event to all imgui contexts
		const auto currentImGuiContext = ImGui::GetCurrentContext();
		for (const auto ctx : instance().m_activeContexts)
		{
			ImGui::SetCurrentContext(ctx->imguiContext);
			ImGui_ImplSDL3_ProcessEvent(&event);
		}
		ImGui::SetCurrentContext(currentImGuiContext);

		// TODO: handle multiple windows
	}
}

void dk::io::GlobalState::tryHandleAndDispatchEvents(WindowEventHandlerBase* evoker)
{
	if (evoker->ticks == state().ticks) 
	{
		// First evoker in tick
		updateNonEventBasedState();
		pollEvents();
		++mutState().ticks;
		details::io::commitInputState(state().inputState);
	}
	evoker->ticks = state().ticks;
}

void dk::io::GlobalState::addListener(SDL_WindowID id, WindowEventHandlerBase* eventHandler)
{
	instance().m_eventHandlers.emplace(id, eventHandler);
}

void dk::io::GlobalState::removeListener(SDL_WindowID id)
{
	instance().m_eventHandlers.erase(id);
}

void dk::io::GlobalState::warpCursorGlobal(const glm::ivec2& location)
{
	SDL_WarpMouseGlobal(location.x, location.y);
	mutState().cursorWarped = 2;
}

void dk::io::GlobalState::warpCursorInWindow(const WindowContext* context, const glm::ivec2& location)
{
	SDL_WarpMouseInWindow(context->sdlWindowContext, location.x, location.y);
	mutState().cursorWarped = 2;
}

void dk::io::GlobalState::handleGlobalEvent(const SDL_Event& event)
{
	auto& iState = mutState().inputState;
	switch (event.type)
	{
	case SDL_EVENT_MOUSE_WHEEL: 
		// Initial wheel state is reset in updateNonEventBasedState()
		if (event.wheel.y > 0) iState.wheel |= (wheel_t)wheel_mask::up;
		if (event.wheel.y < 0) iState.wheel |= (wheel_t)wheel_mask::down;
		iState.wheelDirection = event.wheel.y;
		break;
	default:
		break;
	}
}

void dk::io::GlobalState::dispatchEvent(const SDL_WindowEvent& event)
{
	// Try to find a handler registered for the events windowID and dispatch the event
	auto handlerIt = instance().m_eventHandlers.find(event.windowID);
	if (handlerIt == instance().m_eventHandlers.end()) 
	{
		return;
	}
	handlerIt->second->handle(event);
}

void dk::io::GlobalState::updateNonEventBasedState()
{
	// Set input state
	mutState().inputState = {};
	buildInputState();
	// Set cursor state
	mutState().cursorWarped = std::max(0, state().cursorWarped - 1);
	SDL_GetMouseState(&mutState().cursor.x, &mutState().cursor.y);
}

void dk::io::GlobalState::buildInputState()
{
	bool mouseCaptured = false;
	bool keyboardCaptured = false;
	// TODO: implement for multiple windows
	// Allow ImGui to capture mouse and keyboard
	auto& imguiIO = ImGui::GetIO();
	if (details::io::imguiDoCaptureMouse() && imguiIO.WantCaptureMouse)
		mouseCaptured = true;
	if (details::io::imguiDoCaptureKeyboard() && imguiIO.WantCaptureKeyboard)
		keyboardCaptured = true;

	dk::io::InputState& iState = mutState().inputState;

	if (!mouseCaptured) 
	{
		// Button
		iState.buttons = SDL_GetMouseState(nullptr, nullptr);
	}

	if (!keyboardCaptured) 
	{
		// Modkey
		SDL_Keymod mod = SDL_GetModState();
		if (mod & SDL_KMOD_SHIFT) iState.modkeys |= (modkey_t)modkey_mask::shift;
		if (mod & SDL_KMOD_CTRL)  iState.modkeys |= (modkey_t)modkey_mask::ctrl;
		if (mod & SDL_KMOD_ALT)   iState.modkeys |= (modkey_t)modkey_mask::alt;
		if (mod & SDL_KMOD_CAPS)  iState.modkeys |= (modkey_t)modkey_mask::caps;

		// Key
		const bool* keystate = SDL_GetKeyboardState(nullptr);
		// Numbers
		for (auto i = 0ull; i < 10; ++i) {
			if (keystate[SDL_SCANCODE_1 + i]) 
				iState.keys |= ((key_t)key_mask::_0 << ((i + 1) % 10));
		}
		// Letters
		for (auto i = 0ull; i < 26; ++i) {
			if (keystate[SDL_SCANCODE_A + i])
				iState.keys |= ((key_t)key_mask::a << i);
		}
		// F-Keys
		for (auto i = 0ull; i < 11; ++i)
			if (keystate[SDL_SCANCODE_F1 + i])
				iState.keys |= ((key_t)key_mask::f1 << i);

		// Special
		if (keystate[SDL_SCANCODE_GRAVE])     iState.keys |= (key_t)key_mask::grave;
		if (keystate[SDL_SCANCODE_ESCAPE])    iState.keys |= (key_t)key_mask::esc;
		if (keystate[SDL_SCANCODE_TAB])       iState.keys |= (key_t)key_mask::tab;
		if (keystate[SDL_SCANCODE_DELETE])    iState.keys |= (key_t)key_mask::del;
		if (keystate[SDL_SCANCODE_EXECUTE])   iState.keys |= (key_t)key_mask::enter;
		if (keystate[SDL_SCANCODE_BACKSPACE]) iState.keys |= (key_t)key_mask::backspace;
		if (keystate[SDL_SCANCODE_BACKSLASH]) iState.keys |= (key_t)key_mask::backslash;
	}
}

dk::io::WindowContext::~WindowContext()
{
	if (!sdlWindowContext)
		return;

	// Destroy SDL context
	SDL_DestroyWindow(sdlWindowContext);
	sdlWindowContext = nullptr;
	spdlog::trace("[io] Destroyed window: {}", (int)sdlWindowID);

	// Destroy OpenGL context
	SDL_GL_DestroyContext(sdlGlContext);
	spdlog::trace("[io] Destroyed OpenGL context of window: {}", (int)sdlWindowID);

	GlobalState::removeWindowContext(this);

	// TODO: destroy all contexts
}

void dk::io::WindowContext::makeCurrent() const
{
	SDL_GL_MakeCurrent(sdlWindowContext, sdlGlContext);
	ImGui::SetCurrentContext(imguiContext);
	s_currentContext = this;
}
