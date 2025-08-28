#pragma once
#include <devkit/common/utils.h>
#include <devkit/common/properties.h>
#include <devkit/gfx/viewport.h>

namespace dk::io::properties {
namespace window {
	DK_DECL_DERIVED_PROP(size , glm, ivec2 , 720, 480);
	DK_DECL_DERIVED_PROP(title, std, string, "devkit window");
	enum class border { enabled, disabled };
	enum class mode { windowed, fullscreen };
	enum class theme { light, dark };
	enum class vsync { disabled, retrace, adaptive };
	enum class mouse_grab { disabled, enabled };
	enum class default_dockspace : bool { enabled = 1, disabled = 0 };
}
} // dk::io::properties

namespace details::io {

int toUnderlying(dk::io::properties::window::vsync vsync);

uint32_t toUnderlying(dk::io::properties::window::mode mode);

} // details::io

namespace details::io {

struct SDL_StaticContext;

template <typename D>
using WindowProperties = dk::common::DeferredPropertyCollection<D,
	dk::io::properties::window::size,
	dk::io::properties::window::title,
	dk::io::properties::window::border,
	dk::io::properties::window::mode,
	dk::io::properties::window::theme,
	dk::io::properties::window::vsync,
	dk::io::properties::window::mouse_grab,
	dk::io::properties::window::default_dockspace>;

} // details::io

namespace dk::io {

using TickCounter = long long unsigned;

class WindowContext;

class Frame;


class Window 
	: public details::io::WindowProperties<Window>
{
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

	unsigned int imguiDockspaceID() const;
	
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

	template <typename D, typename E>
	friend void details::common::setProperty(D&, const E&);

	friend struct details::io::SDL_StaticContext;
};

} // dk::io
