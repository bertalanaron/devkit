#pragma once
#include <string>
#include <memory>
#include <optional>
#include <sstream>
#include <variant>

#include "devkit/util.h"
#include "devkit/asset_manager.h"

// TODO: BUG when window is opened with properties.useDarkTheme=true it stays in light theme

// TODO: imgui multiple windows

// TODO: look into natvis: https://learn.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects?view=vs-2022

template <>
struct std::formatter<glm::vec4> : std::formatter<std::string> {
	auto format(const glm::vec4& value, std::format_context& ctx) const {
		std::stringstream sstr;
		sstr << "{" << (int)value.r << "," << (int)value.g << "," << (int)value.b << "," << (int)value.a << "}";
		return std::formatter<std::string>::format(sstr.str(), ctx);
	}
};

struct ImGuiContext;

namespace NS_DEVKIT {

class Cursor;

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
	glm::u32vec2 frameSize() const override;
    void* context() override;

    const Cursor& cursor() const;

	static unsigned long long tickCount();

	static Window& focused();

	ImGuiContext* getImGuiContext() const;

	Properties& properties();
	const Properties& properties() const;

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
	glm::vec4	backgroundColor = colors::black;
	bool		alwaysOnTop		= false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Window::Properties, 
	vsyncEnabled, useDarkTheme, width, height, title, borderEnabled, backgroundColor, alwaysOnTop)

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

    enum class WheelDirection : int {
        None =  0,
        Up   =  1 << 0u,
        Down =  1 << 1u,
        Any  =  Up | Down
    };
	
	static bool clicked(Button button = Any);

	static bool isButtonDown(Button button = Any);

    //static float wheelStart(WheelDirection direction = WheelDirection::Any);

    static float wheel(WheelDirection direction = WheelDirection::Any);
};

NLOHMANN_JSON_SERIALIZE_ENUM(dk::Mouse::Button, {
    { Mouse::Button::Left   , "left"   },
    { Mouse::Button::Middle , "middle" },
    { Mouse::Button::Right  , "right"  },
    { Mouse::Button::Any    , "any"    } });

NLOHMANN_JSON_SERIALIZE_ENUM(dk::Mouse::WheelDirection, {
    { Mouse::WheelDirection::None , "none" },
    { Mouse::WheelDirection::Up   , "up"   },
    { Mouse::WheelDirection::Down , "down" },
    { Mouse::WheelDirection::Any  , "any" } });

}

namespace NS_DEVKIT {

enum class Key : uint8_t {
    A = 4,
    B = 5,
    C = 6,
    D = 7,
    E = 8,
    F = 9,
    G = 10,
    H = 11,
    I = 12,
    J = 13,
    K = 14,
    L = 15,
    M = 16,
    N = 17,
    O = 18,
    P = 19,
    Q = 20,
    R = 21,
    S = 22,
    T = 23,
    U = 24,
    V = 25,
    W = 26,
    X = 27,
    Y = 28,
    Z = 29,

    NUM_0 = 30,
    NUM_1 = 31,
    NUM_2 = 32,
    NUM_3 = 33,
    NUM_4 = 34,
    NUM_5 = 35,
    NUM_6 = 36,
    NUM_7 = 37,
    NUM_8 = 38,
    NUM_9 = 39,

    RETURN     = 40,
    ESCAPE     = 41,
    BACKSPACE  = 42,
    TAB        = 43,
    SPACE      = 44,

    SEMICOLON = 51,

    LEFTBRACKET    = 47,
    RIGHTBRACKET   = 48,

    F1  = 58,
    F2  = 59,
    F3  = 60,
    F4  = 61,
    F5  = 62,
    F6  = 63,
    F7  = 64,
    F8  = 65,
    F9  = 66,
    F10 = 67,
    F11 = 68,
    F12 = 69,

    PRINTSCREEN = 70,
    SCROLLLOCK  = 71,
    PAUSE       = 72,
    INSERT      = 73,

    HOME        = 74,
    PAGEUP      = 75,
    DELETE      = 76,
    END         = 77,
    PAGEDOWN    = 78,
    RIGHT       = 79,
    LEFT        = 80,
    DOWN        = 81,
    UP          = 82,
};

enum class KeyMod {
    NONE = 0x0000,
    LSHIFT = 0x0001,
    RSHIFT = 0x0002,
    LCTRL = 0x0040,
    RCTRL = 0x0080,
    LALT = 0x0100,
    RALT = 0x0200,
    LGUI = 0x0400,
    RGUI = 0x0800,
    NUM = 0x1000,
    CAPS = 0x2000,
    MODE = 0x4000,
    SCROLL = 0x8000,

    CTRL = LCTRL | RCTRL,
    SHIFT = LSHIFT | RSHIFT,
    ALT = LALT | RALT,
};

struct Keyboard : private SingletonBase<Keyboard> {
    static bool isKeyDown(Key key);
    static bool isKeyDown(KeyMod mod);

    static bool keyPressed(Key key);
    static bool keyPressed(KeyMod mod);

    static bool checkMods(KeyMod a, KeyMod b);
};

NLOHMANN_JSON_SERIALIZE_ENUM(KeyMod, {
    { KeyMod::NONE   , "none"   },
    { KeyMod::LSHIFT , "lshift" },
    { KeyMod::RSHIFT , "rshift" },
    { KeyMod::LCTRL  , "lctrl"  },
    { KeyMod::RCTRL  , "rctrl"  },
    { KeyMod::LALT   , "lalt"   },
    { KeyMod::RALT   , "ralt"   },
    { KeyMod::LGUI   , "lgui"   },
    { KeyMod::RGUI   , "rgui"   },
    { KeyMod::NUM    , "num"    },
    { KeyMod::CAPS   , "caps"   },
    { KeyMod::MODE   , "mode"   },
    { KeyMod::SCROLL , "scroll" },
    { KeyMod::CTRL   , "ctrl"   },
    { KeyMod::SHIFT  , "shift"  },
    { KeyMod::ALT    , "alt"    } });

}

namespace NS_DEVKIT {

class Cursor {
public:
    struct DragStart {
        KeyMod	     mod;
        glm::i32vec2 position;
    };

    enum CoordinateSystem { Ndc, Window, Global };

    enum class Region { 
        Outside       = 0, 
        InsideNonEdge = 1 << 0u, 
        EdgeLeft      = 1 << 1u, 
        EdgeRight     = 1 << 2u, 
        EdgeTop       = 1 << 3u, 
        EdgeBottom    = 1 << 4u,
        Edge          = EdgeLeft | EdgeRight | EdgeTop | EdgeBottom,
        Inside        = Edge | InsideNonEdge };

public:
    glm::vec2 position(CoordinateSystem coordSys = Ndc) const;
    glm::vec2 drag(Mouse::Button button, std::optional<KeyMod> oMod = std::nullopt, CoordinateSystem coordSys = Ndc) const;
    //glm::vec2 dragDelta(Mouse::Button button, std::optional<KeyMod> oMod = std::nullopt, CoordinateSystem coordSys = Ndc) const;
    glm::vec2 delta(CoordinateSystem coordSys = Ndc) const;

    bool insideRegion(Region region) const;
    bool enteredRegion(Region region) const;

private:
    glm::i32vec2             m_viewportOffset;
    glm::u32vec2             m_viewportSize;
    std::array<DragStart, 3> m_dragStarts;
    glm::vec2                m_windowPosition;
    glm::vec2                m_prevWindowPosition;

    Cursor(glm::i32vec2 offset = {}, glm::u32vec2 size = {}, 
           std::array<DragStart, 3> dragStarts = {}, glm::vec2 windowPosition = {});

    Cursor& operator=(const Cursor& other);

    // Window coords to ndc
    glm::vec2 pointToNdc(const glm::vec2 point) const;
    // Window coords to ndc
    glm::vec2 vecToNdc(const glm::vec2 vec) const;

    friend class Window;
    friend class Viewport;
};

class Viewport : public IFrameProducer {
public:
    struct Properties;

    Viewport(IFrameProducer& parent);
    Viewport(IFrameProducer& parent, Properties properties);

    Properties& properties();

    const Cursor& cursor() const;

    ~Viewport();

public:
    bool beginFrame() override;
    void endFrame() override;
    glm::u32vec2 frameSize() const override;
    void* context() override;

private:
    struct Impl; std::unique_ptr<Impl> m_impl;
    // TODO: std::optional<FrameBuffer&>
};

struct Viewport::Properties {
    glm::i32vec2 offset{  0,  0 };
    glm::u32vec2 size  { -1, -1 };
    glm::vec4    clearColor{ 0, 0, 0, 1 };
    bool		 doClear = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Viewport::Properties, offset, size);

}

namespace NS_DEVKIT {

struct InputCombination {
    using Activator = std::variant<Key, Mouse::Button, Mouse::WheelDirection, Cursor::Region>;
    using Target = std::variant<std::monostate, std::reference_wrapper<const Cursor>>;

    InputCombination(KeyMod mod, Activator activator);
    InputCombination(Activator activator);

    bool active() const;
    bool activated() const;
    bool deactivated() const;

    template <typename T>
    InputCombination& bind(T& target) { m_target = target; return *this; }

private:
    std::optional<KeyMod> m_mod;
    Activator             m_activator;
    Target                m_target;

public:
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(dk::InputCombination,
        m_mod, m_activator);
};

}
