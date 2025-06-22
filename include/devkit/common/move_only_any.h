#include <devkit/common/utils.h>

namespace dk::common {

struct MoveOnlyAny {
	MoveOnlyAny(uintptr_t type) 
		: m_storage(nullptr)
		, m_type(type) 
	{ }
	MoveOnlyAny() { }
	MoveOnlyAny(MoveOnlyAny&& other) noexcept
		: m_storage(other.m_storage)
		, m_type(other.m_type)
	{
		other.m_storage = nullptr;
		other.m_type = reinterpret_cast<uintptr_t>(nullptr);
	}

	template <typename T, typename... Args>
	void emplace(Args&&... args) 
	{
		static_assert(std::move_constructible<T>, "Type T needs to be move constructable.");

		reset();
		m_storage = new char[sizeof(T)];
		new (m_storage) T(std::forward<Args&&>(args)...);
		m_type = reinterpret_cast<uintptr_t>(&typeid(std::decay_t<T>));
		m_destructor = [this] { get<T>().~T(); };
	}

	template <typename T>
	T& get() 
	{
		if (*typeInfo() != typeid(std::decay_t<T>))
			throw std::bad_any_cast{};
		return *reinterpret_cast<T*>(m_storage);
	}

	template <typename T>
	const T& get() const 
	{
		if (!is_type<T>())
			throw std::bad_any_cast{};
		return *reinterpret_cast<T*>(m_storage);
	}

	bool has_value() const {
		return m_storage != nullptr;
	}

	template <typename T>
	bool is_type() const {
		if (m_type == 0)
			return false;
		return *typeInfo() == typeid(std::decay_t<T>);
	}

	~MoveOnlyAny() { reset(); }
private:
	void*	  m_storage  = nullptr;
	uintptr_t m_type	 = reinterpret_cast<uintptr_t>(nullptr);
	std::function<void()> m_destructor{};

	void reset() {
		if (!m_storage)
			return;

		m_destructor();
		delete m_storage;

		m_destructor = {};
		m_storage = nullptr;
		m_type = reinterpret_cast<uintptr_t>(nullptr);
	}

	const std::type_info* typeInfo() const {
		return reinterpret_cast<const type_info*>(m_type);
	}
};

}
