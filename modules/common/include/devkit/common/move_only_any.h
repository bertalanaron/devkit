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
		return reinterpret_cast<const std::type_info*>(m_type);
	}
};

struct move_only_any {
    explicit move_only_any(std::uintptr_t type)
        : m_type(type)
    { }
    move_only_any() = default;
    move_only_any(move_only_any&& other) noexcept
        : m_storage(other.m_storage)
        , m_size(other.m_size)
        , m_alignment(other.m_alignment)
        , m_type(other.m_type)
        , m_destructor(other.m_destructor)
    {
        other.m_storage = nullptr;
        other.m_size = 0;
        other.m_alignment = alignof(std::max_align_t);
        other.m_type = reinterpret_cast<std::uintptr_t>(nullptr);
        other.m_destructor = nullptr;
    }

	template <typename T>
	move_only_any(T&& object)
        : m_type(reinterpret_cast<std::uintptr_t>(&typeid(std::decay_t<T>)))
	{
		emplace<T>(std::forward<T>(object));
    }

    move_only_any& operator=(move_only_any&& other) noexcept
    {
        if (this == &other)
            return *this;

        reset();
        m_storage = other.m_storage;
        m_size = other.m_size;
        m_alignment = other.m_alignment;
        m_type = other.m_type;
        m_destructor = other.m_destructor;

        other.m_storage = nullptr;
        other.m_size = 0;
        other.m_alignment = alignof(std::max_align_t);
        other.m_type = reinterpret_cast<std::uintptr_t>(nullptr);
        other.m_destructor = nullptr;

        return *this;
    }

    template <typename T, typename... Args>
    void emplace(Args&&... args)
    {
        static_assert(std::move_constructible<T>, "Type T needs to be move constructable.");

        reset();
        void* storage = ::operator new(sizeof(T), std::align_val_t(alignof(T)));
        try {
            new (storage) T(std::forward<Args>(args)...);
        } catch (...) {
            ::operator delete(storage, std::align_val_t(alignof(T)));
            throw;
        }

        m_storage = storage;
        m_size = sizeof(T);
        m_alignment = alignof(T);
        m_type = reinterpret_cast<std::uintptr_t>(&typeid(std::decay_t<T>));
        m_destructor = [](void* storage) { static_cast<T*>(storage)->~T(); };
    }
	
	template <typename T>
	move_only_any& operator=(T&& object)
	{
		emplace<T>(std::forward<T>(object));
    }

    template <typename T>
    T& get()
    {
        if (!is_type<T>())
            throw std::bad_any_cast{};
        return *static_cast<T*>(m_storage);
    }

    template <typename T>
    const T& get() const
    {
        if (!is_type<T>())
            throw std::bad_any_cast{};
        return *static_cast<const T*>(m_storage);
    }

    [[nodiscard]] bool has_value() const {
        return m_storage != nullptr;
    }

    template <typename T>
    [[nodiscard]] bool is_type() const {
        if (m_type == 0)
            return false;
        return *typeInfo() == typeid(std::decay_t<T>);
    }

    ~move_only_any() { reset(); }

private:
    void*                 m_storage = nullptr;
    std::size_t           m_size = 0;
    std::size_t           m_alignment = alignof(std::max_align_t);
    std::uintptr_t        m_type = reinterpret_cast<std::uintptr_t>(nullptr);
    void (*m_destructor)(void*) = nullptr;

    void reset() {
        if (!has_value())
            return;

        if (m_destructor)
            m_destructor(m_storage);
        ::operator delete(m_storage, std::align_val_t(m_alignment));
        m_storage = nullptr;
        m_size = 0;
        m_alignment = alignof(std::max_align_t);
        m_type = reinterpret_cast<std::uintptr_t>(nullptr);
        m_destructor = nullptr;
    }

    [[nodiscard]] const std::type_info* typeInfo() const {
        return reinterpret_cast<const std::type_info*>(m_type);
    }
};

}
