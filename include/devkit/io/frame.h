#pragma once
#include <devkit/common/utils.h>
#include <devkit/io/input_combination.h>
#include <devkit/gfx/viewport.h>

namespace dk::io {

class Frame {
public:
	gfx::Viewport& viewport()
	{ return m_viewport; }

	const gfx::Viewport& viewport() const
	{ return m_viewport; }

	//@brief Binds input state, activates viewport
	void makeCurrent() const;

	glm::ivec2 cursorP() const;
	glm::vec2 cursorN() const;

	glm::ivec2 cursorDeltaP() const;
	glm::vec2 cursorDeltaN() const;

	double aspectRatio() const;

	template <typename _Rep>
	const auto dt() const
	{ return std::chrono::duration<double>(std::chrono::duration_cast<_Rep>(m_dt)).count(); }

	void warpCursor(const glm::ivec2& destination) const;

	Frame(const Frame&          previous, 
		  void*                 producerContext,
		  const gfx::Viewport&  viewport,
		  const io::InputState& inputState,
		  const glm::ivec2&     cursor);

	Frame();

private:
	void*          m_producerContext = nullptr;
	gfx::Viewport  m_viewport;

	io::InputState m_inputState;
	io::InputState m_prevInputState;

	glm::ivec2     m_cursor;
	glm::ivec2     m_prevCursor;

	std::chrono::system_clock::time_point m_t;
	std::chrono::seconds                  m_dt;
};

} // dk::io
