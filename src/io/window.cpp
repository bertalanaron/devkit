#include <devkit/io/window.h>
#include <devkit/io/input_combination.h>
#include <devkit/io/frame.h>
#include "../gfx/context.h"

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

class dk::io::Window::WindowEventHandler
	: public dk::io::WindowEventHandlerBase
{
public:
	WindowEventHandler(Window& window)
		: m_window(window)
	{ }

	void handle(const SDL_WindowEvent& event) override
	{
		switch (event.type)
		{
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED: 
			m_window.m_closeRequested = true;
			break;
		case SDL_EVENT_WINDOW_RESIZED:
			using size_t = dk::io::properties::window::size;
			m_window.propertyChanged(size_t(event.data1, event.data2));
			break;
		default:
			break;
		}
	}

private:
	Window& m_window;
};

void dk::io::Window::open(int msaa)
{
	// Setup context and event handling
	m_context = GlobalState::createWindowContext(msaa, property<properties::window::size>());
	GlobalState::addListener(m_context->sdlWindowID, m_eventHandler.get());
	m_isOpen  = true;
	// Configure OpenGL
	glEnable(GL_PROGRAM_POINT_SIZE);  

	// Configure ImGui
	m_context->imguiIO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	m_context->imguiIO->ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;
	ImGui::StyleColorsDark();

	callPropertySetters();
}

void dk::io::Window::close()
{
	// Tear down context and stop event handling
	GlobalState::removeListener(m_context->sdlWindowID);
	m_context.reset();
	m_isOpen = false;
}

bool dk::io::Window::isOpen() const
{
	return m_isOpen;
}

const dk::io::Frame& dk::io::Window::beginFrame()
{
	DK_ASSERT((m_isOpen, "beginFrame requires the window to be open"));

	// Use context
	m_context->makeCurrent();
	// Handle events
	GlobalState::tryHandleAndDispatchEvents(m_eventHandler.get());
	// Update properties
	callPropertySetters();

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	// Setup dockspace
	ImGui::DockSpaceOverViewport(m_context->imguiDockspaceId, (const ImGuiViewport*)0, 
		ImGuiDockNodeFlags_PassthruCentralNode);

	// Build viewport using imgui dockspace 
	// and apply to backbuffer
	const auto viewport = buildViewport();
	gfx::backBuffer().setViewport(viewport);

	// Create and bind frame
	auto nextFrame = std::make_unique<const Frame>(
		*m_frame, 
		m_context.get(), 
		viewport, 
		GlobalState::getInputStateOfWindow(m_context.get()),
		(glm::ivec2)GlobalState::state().cursor);
	m_frame.swap(nextFrame);
	m_frame->makeCurrent();

	// Return frame
	return *m_frame;
}

void dk::io::Window::endFrame()
{
	if (!m_isOpen)
		return;

	// Use context
	m_context->makeCurrent();

	// Render ImGui draw data
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	// ImGui multi viewports support
	if (m_context->imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		//SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		//SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		//SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}

	m_context->makeCurrent();

	ImGui::EndFrame();

	// Swap buffers
	SDL_GL_SwapWindow(m_context->sdlWindowContext);

	// Close if requested
	if (m_closeRequested)
		close();
}

void dk::io::Window::makeCurrent()
{
	m_context->makeCurrent();
}

void dk::io::Window::warpCursor(const glm::ivec2& destination) const
{
	GlobalState::warpCursorInWindow(m_context.get(), destination);
}

dk::io::Window::Window()
	: m_eventHandler(std::make_unique<WindowEventHandler>(*this))
	, m_frame(std::make_unique<const Frame>())
{ }

dk::io::Window::~Window()
{ }

ImRect GetCentralNodeRect(ImGuiDockNode* node)
{
	// If this is a leaf node
	if (node->IsLeafNode())
	{
		// Central node: no window docked in it
		if ((node->LocalFlags & ImGuiDockNodeFlags_CentralNode) != 0)
			return ImRect(node->Pos, ImVec2(node->Pos.x + node->Size.x, node->Pos.y + node->Size.y));
		else
			return ImRect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX); // Invalid rect
	}

	ImRect rect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (int i = 0; i < IM_ARRAYSIZE(node->ChildNodes); i++)
	{
		if (node->ChildNodes[i])
		{
			ImRect childRect = GetCentralNodeRect(node->ChildNodes[i]);
			if (childRect.Min.x < rect.Min.x) rect.Min.x = childRect.Min.x;
			if (childRect.Min.y < rect.Min.y) rect.Min.y = childRect.Min.y;
			if (childRect.Max.x > rect.Max.x) rect.Max.x = childRect.Max.x;
			if (childRect.Max.y > rect.Max.y) rect.Max.y = childRect.Max.y;
		}
	}
	return rect;
}

dk::gfx::Viewport dk::io::Window::buildViewport() const
{
	// Get dockspace node from context
	ImGuiDockNode* node = ImGui::DockBuilderGetNode(m_context->imguiDockspaceId);
	if (!node)
		std::terminate();

	// Calculate central rect (remaining space after docking)
	ImRect centralRect = GetCentralNodeRect(node);
	if (centralRect.Min.x > centralRect.Max.x) // Invalid rect
		return {};

	// Parse values (convert y coords)
	const ImVec2 size = ImVec2(centralRect.GetWidth(), centralRect.GetHeight());  // Width/Height in pixels
	const ImVec2 pos  = ImVec2(centralRect.Min.x - node->Pos.x, centralRect.Min.y - node->Pos.y);   // Top-left in screen space
	const auto windowSizeY = property<dk::io::properties::window::size>().y;

	return dk::gfx::Viewport(
		glm::ivec2(size.x, size.y), 
		glm::ivec2(pos.x, windowSizeY - (pos.y + size.y)), pos.y);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::size& size) 
{
	SDL_SetWindowSize(window.m_context->sdlWindowContext, size.x, size.y);
	spdlog::trace("Set size for window: {}", window.m_context->sdlWindowID);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::title& title)
{
	SDL_SetWindowTitle(window.m_context->sdlWindowContext, title.c_str());
	spdlog::trace("Set title as \"{}\" for window: {}", title, window.m_context->sdlWindowID);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::border& border)
{
	bool isEnabled = border == dk::io::properties::window::border::enabled;
	SDL_SetWindowBordered(window.m_context->sdlWindowContext, isEnabled);
	spdlog::trace("Set border {} for window: {}", (isEnabled ? "enabled" : "disabled"), window.m_context->sdlWindowID);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::theme& theme)
{
	BOOL USE_DARK_MODE = theme == dk::io::properties::window::theme::dark;
	BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
		window.m_context->nativeWindowHandle, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
		&USE_DARK_MODE, sizeof(USE_DARK_MODE)));
	spdlog::trace("Set {} theme for window: {}", (USE_DARK_MODE ? "dark" : "light"), window.m_context->sdlWindowID);

	// hack: Have to hide and show the window to apply color change
	bool border = window.property<dk::io::properties::window::border>() == dk::io::properties::window::border::enabled;
	SDL_SetWindowBordered(window.m_context->sdlWindowContext, !border);
	SDL_SetWindowBordered(window.m_context->sdlWindowContext, border);
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::vsync& vsync)
{
	SDL_GL_SetSwapInterval(details::io::toUnderlying(vsync));
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::mode& mode)
{
	SDL_SetWindowFullscreen(window.m_context->sdlWindowContext, details::io::toUnderlying(mode));
}

template <>
void details::common::setProperty(dk::io::Window& window, const dk::io::properties::window::mouse_grab& mode)
{
	SDL_SetWindowMouseGrab(window.m_context->sdlWindowContext, (bool)mode);
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
