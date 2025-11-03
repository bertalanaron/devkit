#pragma once
#include <devkit/common/utils.h>

namespace dk::gfx {

enum class Primitive : unsigned int {
	Points, Lines, Triangles, Patches
};

namespace properties {

enum class backface_culling { disabled, enabled };

} // properties

} // dk::gfx

namespace details::gfx {

struct GLType {
	unsigned type  = 0;
	size_t	 size  = 0;
	unsigned count = 0;
    unsigned repeate = 1; // TODO: document

	template <typename T>
	constexpr static GLType get();

	bool operator==(const GLType&) const = default;
};

unsigned int toUnderlying(dk::gfx::Primitive primitive);

} // details::gfx

namespace std {

template <>
struct hash<details::gfx::GLType> {
    std::size_t operator()(const details::gfx::GLType& t) const noexcept {
        std::size_t h1 = std::hash<unsigned>{}(t.type);
        std::size_t h2 = std::hash<size_t>{}(t.size);
        std::size_t h3 = std::hash<unsigned>{}(t.count);
        std::size_t h4 = std::hash<unsigned>{}(t.repeate);

        std::size_t seed = h1;
        seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

template <>
struct hash<std::vector<details::gfx::GLType>> {
    std::size_t operator()(const std::vector<details::gfx::GLType>& vec) const noexcept {
        size_t h = 0;
        for (const auto& t : vec)
            h ^= std::hash<details::gfx::GLType>()(t) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

}

