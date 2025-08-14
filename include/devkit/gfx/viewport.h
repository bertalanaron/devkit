#pragma once
#include <devkit/common/utils.h>
#include <devkit/io/input_combination.h>

namespace dk::gfx {

class Viewport {
public:
	void makeActive() const;

	// Flips y coord
	glm::vec2 normalize(const glm::ivec2& pixels) const;

	double aspectRatio() const;

	const auto& size() const
	{ return m_size; }

	const auto& offset() const
	{ return m_offset; }

	const auto& offsetFromTop() const
	{ return m_offsetTop; }

	Viewport() = default;
	Viewport(const glm::ivec2& size);
	Viewport(const glm::ivec2& size, const glm::ivec2& offset, int offsetTop);

	bool wrapPoint(glm::ivec2& point, int border = 0) const;

	glm::ivec2 nearEdges(const glm::ivec2& point) const;

private:
	glm::ivec2 m_size      = { 0, 0 };
	glm::ivec2 m_offset    = { 0, 0 };
	int        m_offsetTop = 0;
};

}
