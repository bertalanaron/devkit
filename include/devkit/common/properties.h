#pragma once
#include <devkit/common/utils.h>

namespace dk::common {

template <typename T, string_literal Name>
struct UniqueProperty {
	using type = T;

	inline static constexpr auto property_name = Name;

	type value;

	UniqueProperty() = default;

	UniqueProperty(T&& _value)
		: value(std::forward<decltype(_value)>(_value))
	{ }

	template <typename... Ts>
		requires (std::is_constructible_v<T, Ts...>)
	UniqueProperty(Ts&&... initializers)
		: value(std::forward<decltype(initializers)>(initializers)...)
	{ }

	UniqueProperty& operator=(T&& _value)
	{ value = std::forward<decltype(_value)>(_value); }

	operator T&() { return value; }
	operator const T&() const { return value; }
};

template <typename T, string_literal Name>
struct std::formatter<UniqueProperty<T, Name>> : std::formatter<T> {
	template <typename FormatContext>
	auto format(const dk::common::UniqueProperty<T, Name>& prop, FormatContext& ctx) const {
		return std::formatter<T>::format(prop.value, ctx);
	}
};

template <typename T, string_literal Name>
inline void to_json(nlohmann::json& j, const UniqueProperty<T, Name>& p) 
{ to_json(j, p.value); }

template <typename T, string_literal Name>
inline void from_json(const nlohmann::json& j, UniqueProperty<T, Name>& p) 
{ from_json(j, p.value); }

template <typename... Properties>
class ConfigurationBase {
private:
	static_assert(is_each_unique<Properties...>, 
		"Each property must have a unique type");

	using value_type  = std::tuple<Properties...>;
	using dirty_flags = std::bitset<sizeof...(Properties)>;

	template <typename Property>
	inline static constexpr std::size_t property_index = index_of<Property, Properties...>;

public:
	ConfigurationBase() = default;

	template <OfList<Properties...>... Ts>
	ConfigurationBase(Ts&&... properties)
	{ set(std::forward<decltype(properties)>(properties)...); }

	template <OfList<Properties...> Property>
	void operator()(Property&& value)
	{
		constexpr auto index = property_index<Property>;
		std::get<index>(m_value) = std::forward<decltype(value)>(value);
		m_dirty.set(index, true);
	}

	template <OfList<Properties...>... Ts>
	void operator()(Ts&&... values)
	{ (this->operator()(std::forward<decltype(values)>(values)), ...); }

	template <OfList<Properties...>... Ts>
	void set(Ts&&... values)
	{ this->operator()(std::forward<decltype(values)>(values)...); }

	template <OfList<Properties...> Property>
	const auto& get() const
	{
		constexpr auto index = property_index<Property>;
		return std::get<index>(m_value);
	}

	template <OfList<Properties...> First, OfList<Properties...> Second, OfList<Properties...>... Rest>
	auto get() const -> std::tuple<First, Second, Rest...>
	{ return std::make_tuple(get<First>(), get<Second>(), get<Rest>()...); }

	template <typename F>
	void for_each(F&& callable) const 
	{ 
		static_assert((std::is_invocable_v<F, Properties> && ...),
			"Callable must be invokable with each property type.");

		for_each_in_tuple(m_value, callable); 
	}

	template <OfList<Properties...> P>
	bool dirty() const { return m_dirty.test(property_index<P>); }

	template <OfList<Properties...> P>
	bool dirty(const P&) const { return m_dirty.test(property_index<P>); }

	template <OfList<Properties...> P>
	void reset_dirty() { m_dirty.reset(property_index<P>); }

	void reset_dirty() { m_dirty.reset(); }

	template <OfList<Properties...> E>
		requires (std::is_enum_v<E>)
	static std::string_view property_name(const E&)
	{ return magic_enum::enum_type_name<E>(); }

	template <OfList<Properties...> P>
		requires requires { { P::property_name }; }
	static std::string_view property_name(const P&)
	{ return P::property_name; }

private:
	dirty_flags m_dirty;
	value_type  m_value;
};

template <typename... Properties>
inline void to_json(nlohmann::json& j, const ConfigurationBase<Properties...>& config)
{
	config.for_each([&j](const auto& p) { 
		j[ConfigurationBase<Properties...>::property_name(p)] = nlohmann::json(p);
	});
}

template <typename... Properties>
inline void from_json(const nlohmann::json& j, ConfigurationBase<Properties...>& config)
{
	config.for_each([&j,&config](const auto& p) { 
		using P = std::decay_t<decltype(p)>;
		config(std::move(j[ConfigurationBase<Properties...>::property_name(p)].template get<P>()));
	});
}

} // namespace dk::common



namespace _dk::io {

class Window {
public:
	using      Size      = dk::common::UniqueProperty<glm::ivec2 , "Size">;
	using      Title     = dk::common::UniqueProperty<std::string, "Title">;
	enum class Border    { Enabled = 1, Disabled = 0 };
	enum class Mode      { Windowed, Fullscreen };
	enum class Theme     { Light, Dark };
	enum class VSync     { Disabled, Retrace, Adaptive };
	enum class MouseGrab { Disabled, Enabled };

	class Config 
		: private dk::common::ConfigurationBase<
			Size, Title, Border, Mode, Theme, VSync, MouseGrab>
	{
	public:
		using ConfigurationBase::operator();
		using ConfigurationBase::set;
		using ConfigurationBase::get;

		inline friend void to_json(nlohmann::json& j, const Config& config)
		{ to_json(j, (const dk::common::ConfigurationBase<Size, Title, Border, Mode, Theme, VSync, MouseGrab>&)config); }
		
		inline friend void from_json(const nlohmann::json& j, Config& config)
		{ from_json(j, (dk::common::ConfigurationBase<Size, Title, Border, Mode, Theme, VSync, MouseGrab>&)config); }

	private:
		using ConfigurationBase::ConfigurationBase;

		friend class Window;
	};
	
	Config config;

	void beginFrame()
	{
		config.for_each([this](const auto& prop) {
			if (!config.dirty(prop))
				return;
			spdlog::debug("Window.{}={}", Config::property_name(prop), std::format("{}", prop));
			setWindowProperty(*this, prop);
		});
		config.reset_dirty();
	}

	Window()
		: config(Title("Unnamed"), Size(720, 480))
	{ 
		config.reset_dirty(); 
	}

private:
	template <typename P>
	friend void setWindowProperty(Window&, const P&);
};

//class FrameBuffer {
//public:
//	enum class DepthTest { Disabled, Enabled };
//	enum class DepthFunc {  };
//	enum class Blend { Disabled, Enabled };
//	enum class BlendEquation { };
//	enum class CullFace { Disabled, Enabled };
//	enum class ScissorTest { Disabled, Enabled };
//	enum class Multisample { Disabled, Enabled };
//	using      LineWidth = dk::common::UniqueProperty<float, "LineWidth">;
//	using      PointSize = dk::common::UniqueProperty<float, "PointSize">;
//
//	class Config
//		: private dk::common::ConfigurationBase<
//			DepthTest, DepthFunc, Blend, BlendEquation, CullFace, ScissorTest, Multisample, LineWidth, PointSize>
//	{
//	public:
//		using ConfigurationBase::operator();
//		using ConfigurationBase::set;
//		using ConfigurationBase::get;
//
//	private:
//		using ConfigurationBase::ConfigurationBase;
//
//		friend class FrameBuffer;
//	};
//
//	Config config;
//
//private:
//	template <typename P>
//	friend void setFrameBufferProperty(FrameBuffer&, const P&);
//};

class Texture {
public:
	enum class MinFilter { };
	enum class MagFilter { };
	enum class WrapS { };
	enum class WrapT { };
	using      BorderColor = dk::common::UniqueProperty<glm::vec4, "BorderColor">;

	class Config 
		: dk::common::ConfigurationBase<
			MinFilter, MagFilter, WrapS, WrapT, BorderColor>
	{

	};

private:

};


template <>
inline void setWindowProperty(Window& window, const Window::Size& size)
{ spdlog::trace("TODO: set window size {}", std::format("{}", size.value)); }

template <>
inline void setWindowProperty(Window& window, const Window::Title& title)
{ spdlog::trace("TODO: set window title {}", title.value); }

template <>
inline void setWindowProperty(Window& window, const Window::Border& border)
{ spdlog::trace("TODO: set window border {}", magic_enum::enum_name(border)); }

template <>
inline void setWindowProperty(Window& window, const Window::Mode& mode)
{ spdlog::trace("TODO: set window mode {}", magic_enum::enum_name(mode)); }

template <>
inline void setWindowProperty(Window& window, const Window::Theme& theme)
{ spdlog::trace("TODO: set window theme {}", magic_enum::enum_name(theme)); }

template <>
inline void setWindowProperty(Window& window, const Window::VSync& vsync)
{ spdlog::trace("TODO: set window vsync {}", magic_enum::enum_name(vsync)); }

template <>
inline void setWindowProperty(Window& window, const Window::MouseGrab& mouseGrab)
{ spdlog::trace("TODO: set window mouseGrab {}", magic_enum::enum_name(mouseGrab)); }

}


inline void f() 
{
	//enum class E1 { a1, a2 };
	//enum class E2 { b1, b2 };
	//enum class E3 { c1, c2 };
	//using Title = dk::common::UniqueProperty<std::string, "Title">;
	//using Size  = dk::common::UniqueProperty<glm::ivec2, "Size">;
	//struct Config 
	//	: dk::common::ConfigurationBase<E1, E2, Title, Size>
	//{  };
	//Config config;
	//
	//config(E1::a1, E2::b2);
	//config(Title("Hello World!"));

	//nlohmann::json json(config);
	//
	//spdlog::info(nlohmann_extension::smart_dump(json));
	//nlohmann_extension::smart_dump(json);

	using Window = _dk::io::Window;
	using FrameBuffer = _dk::io::FrameBuffer;


	Window      window;
	FrameBuffer frameBuffer;

	window.config(Window::Title("Demo Window"),
				  Window::Theme::Dark);

	frameBuffer.config(FrameBuffer::PointSize(2.f));

	Window window2;
	std::apply(window2.config, window.config.get<Window::Title, Window::Theme>());

	from_json(nlohmann::json(window.config), window2.config);

	spdlog::info(nlohmann_extension::smart_dump(nlohmann::json(window2.config)));
}
