#pragma once
#include <devkit/common/utils.h>

namespace details {

template <typename Byte>
class typeless_ref_base {
public:
    typeless_ref_base(std::span<Byte> object)
        : m_object(object)
    { }

    operator std::span<Byte>() const { return m_object; }

    std::span<Byte> get() const { return *this; }
    std::span<Byte> operator*() const { return *this; }

    size_t size() const { return m_object.size(); }

protected:
    std::span<Byte> m_object;
};

} // namespace details

namespace dk::common {

class typeless_ref 
    : public details::typeless_ref_base<std::byte> 
{ 
public:
    using typeless_ref_base::typeless_ref_base;

    template <typename T>
    typeless_ref(T& object)
        : typeless_ref_base(std::span{reinterpret_cast<std::byte*>(&object), sizeof(T)})
    { }

    template <typename T>
    operator T&() const
    { 
        if (sizeof(T) != size())
            throw std::runtime_error("size of T doesn't match contained object size");
        return *reinterpret_cast<T*>(m_object.data()); 
    }

    using typeless_ref_base::get;

    template <typename T>
    T& get() const
    { 
        if (sizeof(T) != size())
            throw std::runtime_error("size of T doesn't match contained object size");
        return *this; 
    }
};

class typeless_cref
    : public details::typeless_ref_base<const std::byte>
{
public:
    using typeless_ref_base::typeless_ref_base;

    template <typename T>
    typeless_cref(const T& object)
        : typeless_ref_base(std::span{ reinterpret_cast<const std::byte*>(&object), sizeof(T) })
    { }

    template <typename T>
    operator const T&() const
    { 
        if (sizeof(T) != size())
            throw std::runtime_error("size of T doesn't match contained object size");
        return *reinterpret_cast<const T*>(m_object.data()); 
    }

    using typeless_ref_base::get;

    template <typename T>
    const T& get() const
    { 
        if (sizeof(T) != size())
            throw std::runtime_error("size of T doesn't match contained object size");
        return *this; 
    }
};

template <typename T>
constexpr bool is_typeless_ref_v = std::is_same_v<T, typeless_ref> || std::is_same_v<T, typeless_cref>;

class typeless_vector {
public:
    struct iterator {
        using value_type        = typeless_ref;
        using difference_type   = std::ptrdiff_t;

        iterator()                = default;
        iterator(const iterator&) = default;
        iterator(size_t elemSize, std::byte* ptr)
            : m_elemSize(elemSize)
            , m_ptr(ptr)
        { }

        value_type operator*() const { return value_type(as_span()); }

        iterator& operator++()
        { 
            m_ptr += m_elemSize;
            return *this; 
        }

        iterator operator++(int)
        { iterator it = *this; ++(*this); return it; }

        auto operator<=>(const iterator&) const = default;

        std::byte* ptr() const { return m_ptr; }
        size_t elem_size() const { return m_elemSize; }

    private:
        size_t     m_elemSize = 0;
        std::byte* m_ptr      = nullptr;

        std::span<std::byte> as_span(size_t offset = 0) const
        { return { m_ptr, m_elemSize }; }
    };

    struct const_iterator {
        using value_type        = typeless_cref;
        using difference_type   = std::ptrdiff_t;

        const_iterator() = default;
        const_iterator(const const_iterator&) = default;
        const_iterator(size_t elemSize, const std::byte* ptr)
            : m_elemSize(elemSize)
            , m_ptr(ptr)
        { }

        value_type operator*() const { return value_type(as_span()); }

        const_iterator& operator++()
        { 
            m_ptr += m_elemSize;
            return *this; 
        }

        const_iterator operator++(int)
        { const_iterator it = *this; ++(*this); return it; }

        auto operator<=>(const const_iterator&) const = default;

        const std::byte* ptr() const { return m_ptr; }
        size_t elem_size() const { return m_elemSize; }

    private:
        size_t           m_elemSize = 0;
        const std::byte* m_ptr      = nullptr;

        std::span<const std::byte> as_span(size_t offset = 0) const
        { return { m_ptr, m_elemSize }; }
    };

    explicit typeless_vector(size_t elemSize)
        : m_elemSize(elemSize)
    { }

    template <typename T>
    typeless_vector(id_t<T>)
        : m_elemSize(sizeof(T))
    { }

    const std::byte* data() const { return m_data.data(); }

    template <typename T>
    std::span<T> as()
    { 
        if (sizeof(T) != elem_size())
            throw std::runtime_error("size of T doesn't match contained object size");
        return { reinterpret_cast<T*>(m_data.data()), m_size };
    }

    template <typename T>
    std::span<const T> as() const
    { 
        if (sizeof(T) != elem_size())
            throw std::runtime_error("size of T doesn't match contained object size");
        return { reinterpret_cast<const T*>(m_data.data()), m_size };
    }

    void push_back(typeless_cref object)
    {
        DK_ASSERT((object.size() == m_elemSize,
            "Incorrect element size"));
        ++m_size;
        m_data.resize(size() * elem_size());
        std::memcpy(&m_data.data()[(size() - 1) * elem_size()], object.get().data(), elem_size());
    }

    template <typename T>
        requires(!is_typeless_ref_v<T>)
    void push_back(const T& object)
    {
        if (sizeof(T) != elem_size())
            throw std::runtime_error("size of T doesn't match contained object size");
        ++m_size;
        m_data.resize(size() * elem_size());
        new (reinterpret_cast<T*>(&m_data.data()[(size() - 1) * elem_size()])) T(object);
    }

    template <typename T>
        requires(!is_typeless_ref_v<T>)
    void emplace_back(T&& object)
    {
        DK_ASSERT((object.size() == m_elemSize,
            "Incorrect element size"));
        ++m_size;
        m_data.resize(size() * elem_size());
        new (reinterpret_cast<T*>(&m_data.data()[(size() - 1) * elem_size()])) T(std::move(object));
    }

    template <typename T>
        requires(!is_typeless_ref_v<T>)
    void emplace(const_iterator where, T&& object)
    {
        if (sizeof(std::remove_reference_t<T>) != elem_size())
            throw std::runtime_error("size of T doesn't match contained object size");

        // compute index
        const std::byte* base = m_data.data();
        const std::byte* whereptr = where.ptr();
        size_t idx = 0;
        if (m_size > 0) {
            if (whereptr < base || whereptr > base + size() * elem_size())
                throw std::out_of_range("Iterator out of range");
            idx = static_cast<size_t>((whereptr - base) / elem_size());
        } else {
            // empty vector: only valid position is begin()
            if (whereptr != base)
                throw std::out_of_range("Iterator out of range");
            idx = 0;
        }

        // make room
        ++m_size;
        m_data.resize(size() * elem_size());
        std::byte* dst_base = m_data.data();

        // move tail up by one element
        std::memmove(&dst_base[(idx + 1) * elem_size()],
            &dst_base[idx * elem_size()],
            (m_size - 1 - idx) * elem_size());

        // placement-new the object into slot idx
        void* dest = &dst_base[idx * elem_size()];
        new (dest) std::remove_reference_t<T>(std::forward<T>(object));
    }

    template <typename It>
        requires(!is_typeless_ref_v<typename std::iterator_traits<It>::value_type>)
    void insert(const_iterator where, It begin, It end)
    {
        using value_t = typename std::remove_reference_t<typename std::iterator_traits<It>::value_type>;
        constexpr size_t value_size = sizeof(value_t);
        if (value_size != elem_size())
            throw std::runtime_error("size of It::value_t doesn't match contained object size");

        // compute count
        size_t count = static_cast<size_t>(std::distance(begin, end));
        if (count == 0) return;

        auto [idx, dst_base] = move_tail_for_insert(where.ptr(), count);

        // construct new elements from range
        std::byte* writePtr = &dst_base[idx * elem_size()];
        It it = begin;
        for (size_t i = 0; i < count; ++i, ++it) {
            void* dest = writePtr + i * elem_size();
            new (dest) value_t(*it);
        }
    }

    void insert(const_iterator where, const_iterator begin, const_iterator end)
    {
        if (begin.elem_size() != elem_size() || end.elem_size() != elem_size())
            throw std::runtime_error("size of const_iterator doesn't match contained object size");

        size_t count = static_cast<size_t>(std::distance(begin.ptr(), end.ptr())) / elem_size();
        if (count == 0) return;

        auto [idx, dst_base] = move_tail_for_insert(where.ptr(), count);

        // construct new elements from range
        std::byte* writePtr = &dst_base[idx * elem_size()];
        auto it = begin;
        for (size_t i = 0; i < count; ++i, ++it) {
            void* dest = writePtr + i * elem_size();
            std::memcpy(dest, it.ptr(), elem_size());
        }
    }

    void insert(const_iterator where, size_t count, typeless_cref value)
    {
        // TODO: implement
        throw std::runtime_error("Not implemented yet");
    }

    template <typename T>
        requires(!is_typeless_ref_v<T>)
    void insert(const_iterator where, size_t count, const T& value)
    {
        // TODO: implement
        throw std::runtime_error("Not implemented yet");
    }

    void erase(const_iterator where)
    {
        // TODO: implement
        throw std::runtime_error("Not implemented yet");
    }

    void erase(const_iterator _begin, const_iterator _end)
    {
        // TODO: implement
        throw std::runtime_error("Not implemented yet");
    }

    void reserve(size_t newCapacity)
    {
        if (newCapacity > capacity())
            m_data.reserve(newCapacity * elem_size());
    }

    typeless_ref at(size_t index)
    {
        if (index >= m_size)
            throw std::out_of_range("Index is out of range");
        return std::span<std::byte>(&m_data.data()[index * elem_size()], elem_size());
    }

    typeless_cref at(size_t index) const
    {
        if (index >= m_size)
            throw std::out_of_range("Index is out of range");
        return std::span<const std::byte>(&m_data.data()[index * elem_size()], elem_size());
    }

    template <typename T>
    T& at(size_t index)
    {
        if (index >= m_size)
            throw std::out_of_range("Index is out of range");
        return reinterpret_cast<T&>(m_data.data()[index * elem_size()]);
    }

    template <typename T>
    const T& at(size_t index) const
    {
        if (index >= m_size)
            throw std::out_of_range("Index is out of range");
        return reinterpret_cast<const T&>(m_data.data()[index * elem_size()]);
    }

    void clear()
    {
        m_data.clear();
        m_size = 0;
    }

    template <typename T>
    void clear()
    {
        for (T& obj : (*this))
            obj.~T();
        clear();
    }

    bool empty() const
    {
        return size() == 0;
    }

    size_t size() const
    { return m_size; }

    size_t elem_size() const
    { return m_elemSize; }

    size_t capacity() const
    { return m_data.capacity() / elem_size(); }

    iterator begin()
    { return iterator(elem_size(), m_data.data()); }

    iterator end()
    { return iterator(elem_size(), &m_data.data()[size() * elem_size()]); }

    const_iterator cbegin() const
    { return const_iterator(elem_size(), m_data.data()); }

    const_iterator cend() const
    { return const_iterator(elem_size(), m_data.data() + size()*elem_size()); }

    const_iterator begin() const
    { return cbegin(); }

    const_iterator end() const
    { return cend(); }

private:
    size_t                 m_elemSize;
    size_t                 m_size = 0;
    std::vector<std::byte> m_data;

    // @returns a pointer to the data buffer and the index of the insertion
    std::pair<size_t, std::byte*> move_tail_for_insert(const std::byte* insertptr, size_t insertCount)
    {
        const std::byte* base = m_data.data();
        size_t idx = 0;
        if (m_size > 0) {
            if (insertptr < base || insertptr > base + size() * elem_size())
                throw std::out_of_range("Iterator out of range");
            idx = static_cast<size_t>((insertptr - base) / elem_size());
        } else {
            if (insertptr != base)
                throw std::out_of_range("Iterator out of range");
            idx = 0;
        }

        size_t oldSize = m_size;
        m_size += insertCount;
        m_data.resize(size() * elem_size());
        std::byte* dst_base = m_data.data();

        // move old tail up by count elements
        std::memmove(&dst_base[(idx + insertCount) * elem_size()],
            &dst_base[idx * elem_size()],
            (oldSize - idx) * elem_size());

        return std::make_pair(idx, dst_base);
    }
};

static_assert(std::ranges::range<typeless_vector>);
static_assert(std::ranges::input_range<typeless_vector>);

struct typeless_layout {
    struct member {
        size_t offset;
        size_t size;
    };
    std::vector<member> members;
};

class typeless_builder {
public:
    typeless_builder(size_t completeObjectSize)
        : m_size(completeObjectSize)
    {
        reset();
    }

    template <typename T>
        requires(!is_typeless_ref_v<T>)
    void push_back(const T& member)
    {
        static_assert(std::is_trivially_copyable_v<T>, 
            "Type must be trivially copyable");

        constexpr auto alignment = alignof(T);
        constexpr auto size      = sizeof(T);

        // Align to type
        if (m_cursor % alignment != 0)
            m_cursor += alignment - (m_cursor % alignment);

        m_layout.members.push_back(typeless_layout::member{ .offset = m_cursor, .size = size });
        std::memcpy(&m_data.data()[m_cursor], &member, size);
        m_cursor += size;
    }

    void push_back(typeless_cref member, size_t alignment)
    {
        const auto size = member.size();

        // Align to type
        if (m_cursor % alignment != 0)
            m_cursor += alignment - (m_cursor % alignment);

        m_layout.members.push_back(typeless_layout::member{ .offset = m_cursor, .size = size });
        std::memcpy(&m_data.data()[m_cursor], member.get().data(), size);
        m_cursor += size;
    }

    typeless_cref get() const
    { 
        return std::span<const std::byte>{ m_data.data(), m_size };
    }

    auto layout() const { return m_layout; }

    void reset()
    {
        m_data.clear();
        m_data.resize(m_size);
    }

private:
    size_t                 m_size;
    size_t                 m_cursor = 0;
    std::vector<std::byte> m_data;
    typeless_layout        m_layout;
};

} // namespace dk::common
