#pragma once
#include <devkit/common/utils.h>

namespace dk::common {

class RuntimeBuffer {
public:
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

}
