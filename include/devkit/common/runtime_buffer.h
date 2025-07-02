#pragma once
#include <devkit/common/utils.h>

namespace dk::common {

class RuntimeBuffer {
public:
	template <typename T>
	RuntimeBuffer(const std::vector<T>& vector)
		: m_elementSize(sizeof(T))
	{
		static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

		m_data.resize(vector.size() * sizeof(T));
		std::memcpy(m_data.data(), vector.data(), m_data.size());
	}

	template <typename T>
	static RuntimeBuffer create()
	{
		return RuntimeBuffer(sizeof(T));
	}

	static RuntimeBuffer create(size_t elementSize)
	{
		return RuntimeBuffer(elementSize);
	}

	template <typename T>
	void push(const T& object) 
	{
		auto i = m_data.size();
		m_data.resize(m_data.size() + elementSize());
		*reinterpret_cast<T*>(&m_data.data()[i]) = object;
		++m_size;
	}

	void push(const std::vector<uint8_t>& binObject)
	{
		DK_ASSERT(("Object size mismatch", binObject.size() == m_elementSize));
		m_data.insert(m_data.end(), binObject.begin(), binObject.end());
		++m_size;
	}

	void clear()
	{
		m_size = 0;
		m_data.clear();
	}

	template <typename T>
	T& at(size_t index) 
	{
		return *reinterpret_cast<T*>(&m_data.at(index * elementSize()));
	}

	template <typename T>
	const T& at(size_t index) const
	{
		return *reinterpret_cast<const T*>(&m_data.at(index * elementSize()));
	}

	const uint8_t* data() const
	{ return m_data.data(); }

	size_t size() const
	{ return m_size; }

	size_t capacity() const
	{
		return m_data.capacity() / elementSize();
	}

	size_t elementSize() const 
	{ return m_elementSize; }

	void reserve(size_t capacity)
	{
		m_data.reserve(capacity * elementSize());
	}

	RuntimeBuffer(const RuntimeBuffer&) = default;
	RuntimeBuffer(RuntimeBuffer&&) = default;

private:
	const size_t         m_elementSize;
	std::vector<uint8_t> m_data;
	size_t               m_size = 0;

	RuntimeBuffer(size_t elementSize)
		: m_elementSize(elementSize)
	{ }
};




// Won't call constructors unless type is provided and will never call destructors. 
class TypelessBuffer {
private:
	class Iterator {
	private:
		class Access {
		public:
			template <typename T>
			operator T&()
			{ 
				DK_ASSERT((sizeof(T) == m_size, "Invalid type"));
				return *reinterpret_cast<T*>(m_ptr);
			}

			template <typename T>
			operator const T&() const
			{ 
				DK_ASSERT((sizeof(T) == m_size, "Invalid type"));
				return *reinterpret_cast<const T*>(m_ptr);
			}

		private:
			size_t   m_size;
			uint8_t* m_ptr;
		};

	public:
		Iterator(size_t size, uint8_t* data)
			: m_size(size)
			, m_data(data)
		{ }

		size_t size() const
		{ return m_size; }

		uint8_t* get()
		{ return m_data; }

		const uint8_t* get() const
		{ return m_data; }

		template <typename T>
		T* get() 
		{
			DK_ASSERT((sizeof(T) == m_size, "Invalid type"));
			return reinterpret_cast<T*>(m_data);
		}

		template <typename T>
		const T* get() const
		{
			DK_ASSERT((sizeof(T) == m_size, "Invalid type"));
			return reinterpret_cast<const T*>(m_data);
		}

		Iterator& operator=(const Iterator& other)
		{
			DK_ASSERT((other.m_size == m_size, "Invalid type"));
			std::memcpy(m_data, other.m_data, m_size);
			return *this;
		}

		template <typename T>
		Iterator& operator=(const T& other)
		{
			DK_ASSERT((sizeof(T) == m_size, "Invalid type"));
			*get<T>() = other;
			return *this;
		}

		Iterator operator+(size_t offset) const
		{ return Iterator(m_size, m_data + m_size * offset); }

		void operator+=(size_t offset)
		{ m_data += m_size * offset; }

		Iterator& operator++()
		{ *this += 1ull; return *this; }

		Iterator operator++(int)
		{ Iterator it = *this; ++(*this); return it; }

		Access& operator*()
		{ return *reinterpret_cast<Access*>(this); }

		const Access& operator*() const
		{ return *reinterpret_cast<const Access*>(this); }

		bool operator!=(const Iterator& other) const 
		{ return m_data != other.m_data; }

	private:
		const size_t m_size;
		uint8_t*     m_data;
	};

public:
	template <typename T>
	TypelessBuffer(id_t<T>, size_t capacity = 2)
		: m_elemSize(sizeof(T))
		, m_data()
	{
		m_data.reserve(capacity * sizeof(T));
	}

	TypelessBuffer(size_t elem_size, size_t capacity = 2)
		: m_elemSize(elem_size)
		, m_data()
	{
		m_data.reserve(capacity * m_elemSize);
	}

	size_t elem_size() const
	{ return m_elemSize; }

	size_t size() const
	{ return m_data.size() / m_elemSize; }

	size_t capacity() const
	{ return m_data.capacity() / m_elemSize; }

	bool empty() const
	{ return m_data.empty(); }

	uint8_t* data()
	{ return m_data.data(); }

	const uint8_t* data() const
	{ return m_data.data(); }

	template <typename T>
	T* data() 
	{ 
		DK_ASSERT((sizeof(T) == m_elemSize, "Invalid type"));
		return reinterpret_cast<T*>(m_data.data()); 
	}

	template <typename T>
	const T* data() const
	{ 
		DK_ASSERT((sizeof(T) == m_elemSize, "Invalid type"));
		return reinterpret_cast<const T*>(m_data.data()); 
	}

	Iterator at(size_t index)
	{
		return Iterator(m_elemSize, &m_data.at(index * m_elemSize));
	}

	const Iterator at(size_t index) const
	{
		return Iterator(m_elemSize, const_cast<uint8_t*>(&m_data.at(index * m_elemSize)));
	}

	template <typename T>
	T& at(size_t index)
	{
		DK_ASSERT((sizeof(T) == m_elemSize, "Invalid type"));
		return *reinterpret_cast<T*>(&m_data.at(index * m_elemSize));
	}

	template <typename T>
	const T& at(size_t index) const
	{
		DK_ASSERT((sizeof(T) == m_elemSize, "Invalid type"));
		return *reinterpret_cast<const T*>(&m_data.at(index * m_elemSize));
	}

	template <typename T>
		requires(!std::is_pointer_v<T>)
	void push_back(const T& elem)
	{
		DK_ASSERT((sizeof(T) == m_elemSize, "Invalid type"));
		m_data.resize(m_data.size() + m_elemSize);
		auto ptr = m_data.data() + (m_data.size() - m_elemSize);
		new (ptr) T(elem);
	}

	void push_back(const uint8_t* elem)
	{
		m_data.resize(m_data.size() + m_elemSize);
		auto ptr = m_data.data() + (m_data.size() - m_elemSize);
		std::memcpy(ptr, elem, m_elemSize);
	}

	template <typename T>
		requires(!std::is_pointer_v<T>)
	void emplace_back(T&& elem)
	{
		DK_ASSERT((sizeof(T) == m_elemSize, "Invalid type"));
		m_data.resize(m_data.size() + m_elemSize);
		auto ptr = m_data.data() + (m_data.size() - m_elemSize);
		new (ptr) T(std::move(elem));
	}

	void clear()
	{ m_data.clear(); }

	void resize(size_t size)
	{ m_data.resize(size * m_elemSize); }

	void reserve(size_t capacity)
	{ m_data.reserve(capacity * m_elemSize); }

	Iterator begin() 
	{ return at(0); }

	Iterator end()
	{ return ++at(size() - 1); }

	const Iterator cbegin() const
	{ return at(0); }

	const Iterator cend() const
	{ return at(size() - 1) + 1; }

	void insert(Iterator where, const Iterator _begin, const Iterator _end)
	{
		DK_ASSERT((where.size() == m_elemSize, "Mismatched element size"));
		DK_ASSERT((_begin.size() == m_elemSize && _end.size() == m_elemSize, "Mismatched element size"));
		DK_ASSERT((_begin.get() <= _end.get(), "Invalid range"));

		size_t insertPos = (where.get() - m_data.data()) / m_elemSize;
		size_t count = (_end.get() - _begin.get()) / m_elemSize;
		if (count == 0)
			return;

		// Resize buffer to fit new elements
		size_t oldSize = size();
		m_data.resize(m_data.size() + count * m_elemSize);

		// Move existing data after insert position
		uint8_t* dest = m_data.data() + (insertPos + count) * m_elemSize;
		uint8_t* src  = m_data.data() + insertPos * m_elemSize;
		std::memmove(dest, src, (oldSize - insertPos) * m_elemSize);

		// Copy new data
		std::memcpy(m_data.data() + insertPos * m_elemSize, _begin.get(), count * m_elemSize);
	}

private:
	const size_t         m_elemSize;
	std::vector<uint8_t> m_data;
};

template <typename T>
class PointerGuard {
public:
	PointerGuard(T& ref, std::mutex& mut)
		: m_ref(ref)
		, m_guard(mut)
	{ }

	T* operator->() 
	{ return &m_ref; }

	T* get() 
	{ return &m_ref; }

	T& operator*()
	{ return m_ref; }

private:
	T&                          m_ref;
	std::lock_guard<std::mutex> m_guard;
};

template <typename Container>
class ThreadDemuxContainer {
private:
	class Global {
	public:
		Global(ThreadDemuxContainer& owner)
			: m_owner(owner)
		{ }

		auto begin()
		{ return m_owner.m_containers.begin(); }

		auto end()
		{ return m_owner.m_containers.end(); }

		const auto cbegin() const
		{ return m_owner.m_containers.cbegin(); }

		const auto cend() const
		{ return m_owner.m_containers.cend(); }

	private:
		ThreadDemuxContainer& m_owner;
	};

public:
	template <typename Fn>
	ThreadDemuxContainer(Fn&& factory)
		: m_factory(std::forward<decltype(factory)>(factory))
		, m_global(*this)
	{ 
		m_containers.reserve(std::thread::hardware_concurrency());
	}

	Container& local()
	{
		auto it = m_containers.find(std::this_thread::get_id());
		if (it == m_containers.end()) {
			std::lock_guard lock(m_mut);
			it = m_containers.try_emplace(std::this_thread::get_id(), std::move(m_factory())).first;
		}
		return m_containers.at(std::this_thread::get_id()); 
	}
	
	const Container& clocal() const
	{ return m_containers.at(std::this_thread::get_id()); }

	PointerGuard<Global> global()
	{ return PointerGuard<Global>(m_global, m_mut); }

	PointerGuard<const Global> cglobal() const
	{ return PointerGuard<const Global>(m_global, m_mut); }

private:
	std::function<Container()>                     m_factory;
	std::unordered_map<std::thread::id, Container> m_containers;
	Global                                         m_global;
	std::mutex                                     m_mut;
};

}

namespace dk::dbg {

// @brief Dump binary memory content to file
inline void binaryDump(const uint8_t* ptr, size_t size, const std::string& path) 
{
	std::ofstream ofs(path, std::ios::binary);
	if (!ofs.is_open()) {
		spdlog::error("Failed to open binary file for writing");
		return;
	}

	ofs.write(reinterpret_cast<const char*>(ptr), size);
	ofs.close();
}

// @brief Requires ImHex installed and added to Path environment variable
inline void peekBinary(const uint8_t* ptr, size_t size, const std::string& label = "debug_dump")
{
	// Write binary data to a temp file
	std::string filename = label + ".bin";
	binaryDump(ptr, size, filename);

	// Open ImHex with the generated file
	std::string command = "imhex \"" + filename + "\"";
	std::system(command.c_str());
}

}
