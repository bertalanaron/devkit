#pragma once
#include <devkit/common/utils.h>
#include <devkit/io/input_combination.h>

namespace dk::gfx {

class Viewport {
public:
	void makeActive() const;

	// Flips y coord
	glm::vec2 normalize(const glm::ivec2& pixels) const;

	const auto& size() const
	{ return m_size; }

	const auto& offset() const
	{ return m_offset; }

	Viewport(const glm::ivec2& size, const glm::ivec2& offset);

	bool wrapPoint(glm::ivec2& point, int border = 0) const;

private:
	glm::ivec2 m_size;
	glm::ivec2 m_offset;
};

}
