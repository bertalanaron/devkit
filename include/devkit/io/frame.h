#pragma once
#include <devkit/common/utils.h>
#include <devkit/io/input_combination.h>
#include <devkit/gfx/viewport.h>
#include <devkit/io/window.h>

namespace dk::io {

class Frame {
public:
	gfx::Viewport& viewport()
	{ return m_viewport; }

	const gfx::Viewport& viewport() const
	{ return m_viewport; }

	const auto& inputState() const
	{ return m_inputState; }

	//@brief Binds input state, activates viewport
	void makeCurrent() const;

	glm::ivec2 cursorP() const;
	glm::vec2 cursorN() const;

	glm::ivec2 cursorDeltaP() const;
	glm::vec2 cursorDeltaN() const;

	double aspectRatio() const;

	template <typename T>
	double dt() const
	{
		using Rep    = typename decltype(m_dt)::rep;
		using Period = typename decltype(m_dt)::period;
		using DblTo  = std::chrono::duration<double, typename T::period>;

		return std::chrono::duration_cast<DblTo>(m_dt).count();
	}

	void warpCursor(const glm::ivec2& destination) const;

	Frame(const Frame&          previous, 
		  const WindowContext*  producerContext,
		  const gfx::Viewport&  viewport,
		  const io::InputState& inputState,
		  const glm::ivec2&     cursor);

	Frame();

protected:
	const WindowContext* m_producerContext = nullptr;
	gfx::Viewport        m_viewport;

	io::InputState       m_inputState;
	io::InputState       m_prevInputState;

	glm::ivec2           m_cursor;
	glm::ivec2           m_prevCursor;

	std::chrono::system_clock::time_point m_t;
	std::chrono::nanoseconds              m_dt;
};

} // dk::io
