#pragma once
#include <string>
#include <memory>
#include <optional>
#include <sstream>

#include "devkit/util.h"
#include "devkit/asset_manager.h"

// TODO: BUG when window is opened with properties.useDarkTheme=true it stays in light theme

// TODO: imgui multiple windows

namespace NS_DEVKIT {

struct Color {
	unsigned char r, g, b, a;

	bool operator==(const Color& other) const { return r == other.r && g == other.g && b == other.b && a == other.a; }
};

}

template <>
struct std::formatter<NS_DEVKIT::Color> : std::formatter<std::string> {
	auto format(const NS_DEVKIT::Color& value, std::format_context& ctx) const {
		std::stringstream sstr;
		sstr << "{" << (int)value.r << "," << (int)value.g << "," << (int)value.b << "," << (int)value.a << "}";
		return std::formatter<std::string>::format(sstr.str(), ctx);
	}
};

struct ImGuiContext;

namespace NS_DEVKIT {

class Window : public IFrameProducer {
public:
	enum class WindowStatus { Ok = 0x00, GraphicsInitError, WindowAllreadyOpen };
	struct Properties;

	Window();
	Window(Properties properties);

	WindowStatus open();
	bool isOpen() const;
	static bool isAnyOpen();
	void close();

	bool beginFrame() override;
	void endFrame() override;

	static unsigned long long tickCount();

	static Window& focused();

	ImGuiContext* getImGuiContext() const;

	Properties& properties();

	~Window();
private:
	struct Impl; std::unique_ptr<Impl> impl;
};

struct Window::Properties { 
	bool		vsyncEnabled    = true;
	bool		useDarkTheme    = false;
	unsigned	width		    = 720;
	unsigned	height		    = 480;
	std::string title		    = "Untitled";
	bool		borderEnabled   = true; 
	Color		backgroundColor = Color{ 0, 0, 0, 255 };
	bool		alwaysOnTop		= false;
};

bool ImGuiAnyHovered();

}

namespace NS_DEVKIT {

class Mouse : private SingletonBase<Mouse> {
public:
	enum Button : uint8_t { 
		Left	= 1 << 0u, 
		Middle	= 1 << 1u, 
		Right	= 1 << 2u, 
		Any		= Left | Middle | Right 
	};
	
	static bool clicked(Button button = Any);

	static bool isButtonDown(Button button = Any);
};

}
