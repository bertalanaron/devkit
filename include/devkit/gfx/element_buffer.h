#pragma once
#include <devkit/common/utils.h>

namespace dk::gfx {

class FrameBuffer;

class ElementBuffer {
public:
	using index_t = unsigned int;
	using face_t  = std::array<index_t, 3>;

	void clear();

	void push(const index_t& index);
	void push(const face_t& face);

private:
	std::vector<index_t> m_indices;

	unsigned m_vao = 0;
	unsigned m_ebo = 0;
	bool     m_resized = true;

	void init();
	void makeActive();
	unsigned count();

	friend class FrameBuffer;
};

}
