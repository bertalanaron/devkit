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
	using      Size      = common::UniqueProperty<glm::ivec2 , "Size">;
	using      Title     = common::UniqueProperty<std::string, "Title">;
	enum class Border    { Enabled = 1, Disabled = 0 };
	enum class Mode      { Windowed, Fullscreen };
	enum class Theme     { Light, Dark };
	enum class VSync     { Disabled, Retrace, Adaptive };
	enum class MouseGrab { Disabled, Enabled };

	class Config 
		: private common::ConfigurationBase<Size, Title, Border, Mode, Theme, VSync, MouseGrab>
	{
	private:
		using Base = common::ConfigurationBase<Size, Title, Border, Mode, Theme, VSync, MouseGrab>;

	public:
		using ConfigurationBase::operator();
		using ConfigurationBase::set;
		using ConfigurationBase::get;

		inline friend void to_json(nlohmann::json& j, const Config& config)
		{ to_json(j, (const Base&)config); }

		inline friend void from_json(const nlohmann::json& j, Config& config)
		{ from_json(j, (Base&)config); }

	private:
		using ConfigurationBase::ConfigurationBase;

		friend class Window;
	};

public:
	Config config;

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
