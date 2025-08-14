#include <devkit/io/frame.h>
#include "../gfx/context.h"

#include <SDL3/SDL.h>

void dk::io::Frame::makeCurrent() const
{
	m_producerContext->makeCurrent();
	details::io::commitInputState(m_prevInputState);
	details::io::commitInputState(m_inputState);
}

glm::ivec2 dk::io::Frame::cursorP() const
{
	return m_cursor;
}

glm::vec2 dk::io::Frame::cursorN() const
{
	return m_viewport.normalize(m_cursor);
}

glm::ivec2 dk::io::Frame::cursorDeltaP() const
{
	return m_cursor - m_prevCursor;
}

glm::vec2 dk::io::Frame::cursorDeltaN() const
{
	return m_viewport.normalize(m_cursor) - m_viewport.normalize(m_prevCursor);
}

void dk::io::Frame::warpCursor(const glm::ivec2& destination) const
{
	GlobalState::warpCursorInWindow(m_producerContext, destination);
}

dk::io::Frame::Frame(
	const Frame&          previous, 
	const WindowContext*  producerContext,
	const gfx::Viewport&  viewport, 
	const io::InputState& inputState, 
	const glm::ivec2&     cursor)
	: m_producerContext(producerContext)
	, m_viewport(viewport)
	, m_inputState(inputState)
	, m_prevInputState(previous.m_inputState)
	, m_cursor(cursor)
	, m_prevCursor(previous.m_cursor)
	, m_t(std::chrono::system_clock::now())
	, m_dt(std::chrono::duration_cast<std::chrono::seconds>(m_t - previous.m_t))
{
	if (GlobalState::state().cursorWarped)
		m_prevCursor = m_cursor;
}

dk::io::Frame::Frame()
	: m_viewport({ 0, 0 }, { 0, 0 }, 0)
	, m_t(std::chrono::system_clock::now())
	, m_dt(std::chrono::seconds(0))
{ }
