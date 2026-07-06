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

	bool operator==(const UniqueProperty&) const = default;

	operator T&() { return value; }
	operator const T&() const { return value; }
};

template <typename T>
struct is_unique_property : std::false_type {};

template <typename U, string_literal Name>
struct is_unique_property<UniqueProperty<U, Name>> : std::true_type {};

template <typename T>
concept UniquePropertySpecialization = is_unique_property<T>::value;

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
	void operator()(const Property& value)
	{
		constexpr auto index = property_index<Property>;
		auto& elem = std::get<index>(m_value);
		const bool changed = !(elem == value);
		elem = std::forward<decltype(value)>(value);
		if (changed)
			m_dirty.set(index, true);
	}

	template <OfList<Properties...> Property>
	void set(const Property& value)
	{
		constexpr auto index = property_index<Property>;
		auto& elem = std::get<index>(m_value);
		const bool changed = !(elem == value);
		elem = std::forward<decltype(value)>(value);
		if (changed)
			m_dirty.set(index, true);
	}

	template <OfList<Properties...>... Ts>
	void operator()(const Ts&... values)
	{ (this->operator()(std::forward<decltype(values)>(values)), ...); }

	template <OfList<Properties...>... Ts>
	void set(const Ts&... values)
	{ (this->set(std::forward<decltype(values)>(values)), ...); }

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

	template <OfList<Properties...> E>
		requires (std::is_enum_v<E>)
	static const E& property_value(const E& e)
	{ return e; }

	template <OfList<Properties...> P>
		requires requires { { P::property_name }; }
	static const typename P::type& property_value(const P& p)
	{ return p.value; }

private:
	dirty_flags m_dirty;
	value_type  m_value;
};

namespace details {

template <typename T>
struct is_derived_from_configurationbase_specialization : std::false_type {};

template <typename T>
	requires (common::is_specialization_of<typename T::Base, ConfigurationBase>)
struct is_derived_from_configurationbase_specialization<T> : std::true_type {};

}

template <typename T>
concept ConfigurationSpecialization = details::is_derived_from_configurationbase_specialization<T>::value;

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

template <typename T, dk::common::string_literal Name>
struct std::formatter<dk::common::UniqueProperty<T, Name>> : std::formatter<T> {
	template <typename FormatContext>
	auto format(const dk::common::UniqueProperty<T, Name>& prop, FormatContext& ctx) const {
		return std::formatter<T>::format(prop.value, ctx);
	}
};

#define DK_CONFIG_SPECIALIZATION(owner, ...)                        \
	 private dk::common::ConfigurationBase<__VA_ARGS__> {           \
	private:                                                        \
	using Base = dk::common::ConfigurationBase<__VA_ARGS__>;	    \
																	\
	public:															\
		using ConfigurationBase::operator();						\
		using ConfigurationBase::set;								\
		using ConfigurationBase::get;								\
		using ConfigurationBase::for_each;							\
		using ConfigurationBase::property_name;                     \
		using ConfigurationBase::property_value;                    \
																	\
		inline friend void to_json(nlohmann::json& j, const Config& config) \
		{ to_json(j, (const Base&)config); }						\
																	\
		inline friend void from_json(const nlohmann::json& j, Config& config) \
		{ from_json(j, (Base&)config); }							\
																	\
	private:														\
		using ConfigurationBase::ConfigurationBase;					\
																	\
		template <typename T>                                       \
		friend struct dk::common::details::is_derived_from_configurationbase_specialization;\
		friend class owner;                                         \
	} 																\
	/* end of macro */
