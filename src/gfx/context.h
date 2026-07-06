#pragma once
#include <devkit/common/utils.h>
#include <devkit/io/input_combination.h>
#include <devkit/io/window.h>

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_system.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#if defined(SDL_PLATFORM_WIN32)
using WindowHandle = HWND;
#endif

namespace dk::io {

class WindowEventHandlerBase {
public:
	virtual void handle(const SDL_WindowEvent& event) = 0;
	TickCounter ticks = 0;
};

class GlobalState 
	: private dk::common::SingletonBase<GlobalState>
{
public:
	struct HardwareVariantState {
		int glMaxTextureImageUnits = 0;
		int glMaxTextureSize       = 0;
		int glMaxTextureLayers     = 0;
		int glMaxColorAttachments  = 0;
	};

	struct State {
		bool        initialized = false;
		TickCounter ticks       = 0;

		InputState inputState;
		glm::vec2  cursor;
		int        cursorWarped = false;

		unsigned glVersionMajor = 4;
		unsigned glVersionMinor = 4;
		unsigned glDepthSize    = 24;
	};

public:
	static const State& state();
	static const HardwareVariantState& hardware();

	static std::unique_ptr<const WindowContext> createWindowContext(int msaa, const glm::ivec2& size);
	static void removeWindowContext(const WindowContext* context);
	static const WindowContext* currentWindowContext();

	static InputState getInputStateOfWindow(const WindowContext*);

	static void showDebugWindow();

	// Event handling and dispatch
	static void tryHandleAndDispatchEvents(WindowEventHandlerBase* evoker);
	static void addListener(SDL_WindowID id, WindowEventHandlerBase* eventHandler);
	static void removeListener(SDL_WindowID id);

	static void warpCursorGlobal(const glm::ivec2& location);
	static void warpCursorInWindow(const WindowContext* context, const glm::ivec2& location);

private:
	State                                                     m_state;
	HardwareVariantState                                      m_harwareVariantState;
	std::unordered_map<SDL_WindowID, WindowEventHandlerBase*> m_eventHandlers;
	std::unordered_set<const WindowContext*>                  m_activeContexts;

	static State& mutState();
	static HardwareVariantState& mutHardware();

	static void tryInitialize();

	static void pollEvents();
	static void handleGlobalEvent(const SDL_Event& event);
	static void dispatchEvent(const SDL_WindowEvent& event);

	static void updateNonEventBasedState();
	static void buildInputState();
};

class WindowContext {
public:
	int           msaa             = 1;
	SDL_Window*   sdlWindowContext = nullptr;
	SDL_GLContext sdlGlContext     = nullptr;
	SDL_WindowID  sdlWindowID;
	ImGuiContext* imguiContext     = nullptr;
	ImGuiID       imguiDockspaceId;
	ImGuiIO*      imguiIO          = nullptr;
#if defined(SDL_PLATFORM_WIN32)
	WindowHandle  nativeWindowHandle;
#endif

	~WindowContext();

	void makeCurrent() const;

private:
	WindowContext() = default;

	inline static const WindowContext* s_currentContext = nullptr;

	friend class GlobalState;
};

} // dk::io
