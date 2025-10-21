#pragma once
#include <devkit/common/utils.h>
#include <devkit/common/properties.h>
#include <devkit/gfx/viewport.h>

namespace dk::io {

using TickCounter = long long unsigned;
class WindowContext;
class Frame;

class Window 
{
public:
	using      Size        = common::UniqueProperty<glm::ivec2 , "Size">;
	using      Title       = common::UniqueProperty<std::string, "Title">;
	enum class Border      { Enabled, Disabled };
	enum class Mode        { Windowed, Fullscreen };
	enum class Theme       { Light, Dark };
	enum class VSync       { Disabled, Retrace, Adaptive };
	enum class MouseGrab   { Disabled, Enabled };
	enum class Resize      { Enabled, Disabled };
	enum class AlwaysOnTop { Disabled, Enabled };
	using      Opacity     = common::UniqueProperty<float, "Opacity">;

	class Config : DK_CONFIG_SPECIALIZATION(Window,
		Size, Title, Border, Mode, Theme, VSync, MouseGrab, 
		Resize, AlwaysOnTop, Opacity);

	Config config;

public:
	Window();

	// @brief Opens a window and activates it's context
	// @param msaa - Multisample Anti Aliasing sample count (msaa is disabled when set to 1)
	void open(int msaa = 1);

	bool isOpen() const;
	
	void close();

	// @brief Begin new frame
	// @returns True when window is open and context switching was successful
	const Frame& beginFrame();

	void endFrame();

	void makeCurrent();

	void warpCursor(const glm::ivec2& destination) const;

	~Window();

private:
	class WindowEventHandler;

private:
	std::unique_ptr<const WindowContext> m_context;
	std::unique_ptr<WindowEventHandler>  m_eventHandler;

	bool                         m_isOpen = false;
	bool                         m_closeRequested = false;
	std::unique_ptr<const Frame> m_frame;
 
	gfx::Viewport buildViewport() const;

	template <typename P>
	friend void setWindowProperty(Window& window, const P&);
};

} // dk::io
