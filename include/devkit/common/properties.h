#pragma once
#include <devkit/common/utils.h>

#define DK_DECL_DERIVED_PROP(name, ns, base, ...)                \
	struct name : ns::base {                                     \
		using type = ns::base;                                   \
        using ns::base::base;                                    \
        name() : ns::base(__VA_ARGS__) { }                       \
		name(const ns::base& v) : ns::base(v) { }                \
		inline static std::string Name = #name;                  \
    }                                                            \
	/* */

namespace details::common {

template <typename D, typename Property>
void setProperty(D& object, const Property& property);

template <typename P>
	requires(std::is_enum_v<P>)
constexpr const char* propertyName() {
	return magic_enum::enum_type_name<P>().data();
}

template <typename P>
	requires(!std::is_enum_v<P>)
constexpr const char* propertyName() {
	return P::Name.data();
}

}

namespace dk::common {

template <typename D, typename... Es>
class DeferredPropertyCollection {
private:
	struct storage_t {
		using setter_t = std::function<void(const std::any&)>;

		std::any data;
		setter_t setter;
		bool     updated;
	};
	
public:
	template <dk::common::OfList<Es...> E>
	void property(E prop) 
	{
		// Return if data hasn't changed
		auto it = m_properties.find(typeid(E));
		if (it != m_properties.end() && std::any_cast<E>(it->second.data) == prop)
			return;
			
		// Insert or update property 
		m_properties[typeid(E)] = storage_t{
			.data = prop,
			.setter = [=](const std::any& data) {
				E prop = std::any_cast<E>(data);
				details::common::setProperty<D, E>(*static_cast<D*>(this), prop); 
			},
			.updated = true
		};
	}

	template <dk::common::OfList<Es...> E>
	E property() 
	{
		auto it = m_properties.find(typeid(E));
		if (it != m_properties.end())
			return std::any_cast<E>(it->second.data);
		return {};
	}

	// @brief Calls the provided functor with a reference to the container and an id type of each parameter
	template <typename F>
	void foreachParam(F f) 
	{
		(f(static_cast<D&>(*this), id_t<Es>{}), ...);
		//foreachParamImpl<F, Es...>(f);
	}

protected:
	void callPropertySetters(bool force = false) {
		for (auto& [_, storage] : m_properties) {
			if (!force && !storage.updated)
				continue;
			storage.setter(storage.data);
			storage.updated = false;
		}
	}

	//template <typename E>
	//	requires(requires { typename E::type; })
	//auto getId_t() {
	//	return id_t<typename E::type>{};
	//}

	//template <typename E>
	//	requires(not requires { typename E::type; })
	//auto getId_t() {
	//	return id_t<E>{};
	//}

	//template <typename F, typename... Ess>
	//void foreachParamImpl(F f) {
	//	(f(static_cast<D&>(*this), getId_t<Ess>()), ...);
	//}

	template <dk::common::OfList<Es...> E>
	void propertyChanged(E prop) 
	{
		// Update if exists
		auto it = m_properties.find(typeid(E));
		if (it != m_properties.end()) {
			it->second.data = prop;
			return;
		}

		// Insert or update property 
		m_properties[typeid(E)] = storage_t{
			.data = prop,
			.setter = [=](const std::any& data) {
				E prop = std::any_cast<E>(data);
				details::common::setProperty<D, E>(*static_cast<D*>(this), prop); 
			},
			.updated = false
		};
	}

private:
	std::unordered_map<std::type_index, storage_t> m_properties;
};

}
