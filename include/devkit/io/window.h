#pragma once
#include <devkit/common/utils.h>
#include <devkit/common/properties.h>

namespace dk::io::properties {
	namespace window {
		DK_DECL_DERIVED_PROP(size , glm, ivec2 , 720, 480);
		DK_DECL_DERIVED_PROP(title, std, string, "devkit window");
		enum class border { enabled, disabled };
		enum class mode { windowed, fullscreen };
		enum class theme { light, dark };
		enum class vsync { disabled, retrace, adaptive };
		enum class mouse_grab { disabled, enabled };
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
	dk::io::properties::window::mouse_grab>;

} // details::io

namespace dk::io {

class Window 
	: public details::io::WindowProperties<Window>
{
public:
	Window();

	// @brief Opens a window and activates it's context
	void open();
	void close();

	// @brief Begin new frame
	// @returns True when window is open and context switching was successful
	bool beginFrame();

	//
	void endFrame();

	// @brief Activates the windows context for graphics (gl context) 
	void useContext();

	std::chrono::nanoseconds dt() const;

	~Window();

private:
	struct Context;

private:
	std::unique_ptr<Context> m_context;

	std::chrono::system_clock::time_point m_frameStart = {};
	std::chrono::system_clock::duration   m_timeSinceLastFrame = {};

	template <typename D, typename E>
	friend void details::common::setProperty(D&, const E&);

	friend struct details::io::SDL_StaticContext;
};

} // dk::io
