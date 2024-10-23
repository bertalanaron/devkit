#pragma once
#include <string>
#include <memory>

#define NS_DEVKIT dk

namespace NS_DEVKIT {

template <typename D>
class SingletonBase {
public:
	SingletonBase() = default;
	SingletonBase(const SingletonBase&) = delete;
	SingletonBase& operator=(const SingletonBase&) = delete;
	
	static D& instance() {
		static D c_instance;
		return c_instance;
	}
};

class Window {
public:
	enum class WindowStatus { Ok = 0x00, GraphicsInitError };
	struct Properties;

	Window(Properties properties);

	WindowStatus open();

	bool isOpen();

	void render();

	Properties& properties();

	~Window();
private:
	struct Impl; std::unique_ptr<Impl> impl;
};

struct Window::Properties { 
	bool		vsyncEnabled  = true;
	bool		useDarkTheme  = false;
	unsigned	width		  = 1280;
	unsigned	height		  = 720;
	std::string title		  = "Untitled";
	bool		borderEnabled = true; 
};

}
