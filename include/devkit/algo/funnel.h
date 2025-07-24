#pragma once
#include <devkit/algo/geometry.h>

// TEMP
#include <devkit/gfx/vertex_sink.h>

namespace dk::algo {

class Funnel {
public:
	using Vertex = glm::dvec2;

	enum class Side : bool 
	{ Left  = false, Right = true };

public:
	Vertex             apex;
	std::deque<Vertex> leftQueue;
	std::deque<Vertex> rightQueue;
	std::deque<Vertex> tail;

public:
	Funnel() = default;

	Funnel(const Funnel& other, const geom::edge2& portal) 
		: apex(other.apex)
		, leftQueue(other.leftQueue)
		, rightQueue(other.rightQueue)
		, m_apexMoved(false)
	{ 
		appendPortal(portal);
	}

	Funnel(const glm::dvec2& start, const geom::edge2& firstPortal) 
		: apex(start)
		, leftQueue({ firstPortal.at(0) })
		, rightQueue({ firstPortal.at(1) })
		, m_apexMoved(false)
	{ }

	void evaluateContainedPaths();

	double min() const
	{ return m_minCost; }

	double max() const
	{ return m_maxCost; }

	void appendPortal(const geom::edge2& portal)
	{
		if (tryAdvanceState<Side::Left>(portal.at(0))) {
			appendVertex<Side::Right>(portal.at(1));
			tryMoveApex<Side::Right>();
			return;
		}
		if (tryAdvanceState<Side::Right>(portal.at(1))) {
			appendVertex<Side::Left>(portal.at(0));
			tryMoveApex<Side::Left>();
			return;
		}
	}

	geom::edge2 endPortal() const 
	{
		if (leftQueue.empty() && rightQueue.empty())
			return geom::nullEdge2;
		if (leftQueue.empty())
			return { apex, rightQueue.back() };
		if (rightQueue.empty())
			return { apex, leftQueue.back() };
		return { leftQueue.back(), rightQueue.back() };
	}

private:
	bool   m_apexMoved = false;

	double m_minCost = std::numeric_limits<double>::infinity();
	double m_maxCost = std::numeric_limits<double>::infinity();

private:
	// @brief True for right
	inline friend constexpr bool to_bool(Side side)
	{ return (bool)side; }

	template <Side side>
	constexpr static Side otherSide = Side(!to_bool(side));

	template <Side side>
	std::deque<Vertex>& selectSide();

	template <>
	std::deque<Vertex>& selectSide<Side::Left>()
	{ return leftQueue; }

	template <>
	std::deque<Vertex>& selectSide<Side::Right>()
	{ return rightQueue; }

	template <Side side>
	const std::deque<Vertex>& selectSide() const;

	template <>
	const std::deque<Vertex>& selectSide<Side::Left>() const
	{ return leftQueue; }

	template <>
	const std::deque<Vertex>& selectSide<Side::Right>() const
	{ return rightQueue; }

	template <Side side>
	void appendVertex(const glm::dvec2& newVertex) 
	{
		if (selectSide<side>().empty() || selectSide<side>().back() != newVertex)
			selectSide<side>().push_back(newVertex);
		ensureConvexity<side>();
	}

	template <Side side>
	Vertex& lastVertexOnSide()
	{ return selectSide<side>().back(); }

	template <Side side>
	const Vertex& lastVertexOnSide() const
	{ return selectSide<side>().back(); }

	template <Side side>
	Vertex& oneBeforeLastVertexOnSide()
	{ return selectSide<side>().at((int)selectSide<side>().size() - 2); }

	template <Side side>
	Vertex& twoBeforeLastVertexOnSideOrApex()
	{ 
		if (selectSide<side>().size() > 2)
			return selectSide<side>().at(selectSide<side>().size() - 3);
		return apex;
	}

	/*
	 *  2 xx      xx 2
	 *       1   1
	 *  CCW   x x   CW
	 *         A
	 */

	template <Side side>
	static constexpr bool isTrigConvexOnSide(const Vertex& a, const Vertex& b, const Vertex& c)
	{
		if constexpr (side == Side::Right)
			return geom::signedTriangle2Area(a, b, c) > 0.0;
		else
			return geom::signedTriangle2Area(a, b, c) < 0.0;
	}

	template <Side side>
	void ensureConvexity()
	{
		while (selectSide<side>().size() > 1
			&& isTrigConvexOnSide<side>(twoBeforeLastVertexOnSideOrApex<side>(), oneBeforeLastVertexOnSide<side>(), lastVertexOnSide<side>()))
		{
			auto front = selectSide<side>().back();
			selectSide<side>().pop_back();
			selectSide<side>().pop_back();
			selectSide<side>().push_back(front);
		}
	}

	template <Side side>
	void tryMoveApex() {
		while (selectSide<side>().size() > 0
			&& (!isTrigConvexOnSide<side>(apex, selectSide<side>().front(), selectSide<otherSide<side>>().front())))
		{
			tail.push_back(apex);
			m_apexMoved = true;
			apex = selectSide<side>().front();
			selectSide<side>().pop_front();
		}
	}

	template <Side side>
	bool tryAdvanceState(const glm::dvec2& newVertex) {
		// First vertex on each side is trivial
		if (selectSide<side>().empty()) {
			selectSide<side>().push_back(newVertex);
			return false;
		}

		// Check if new vertex on side is closer to the inside of the funnel than the previous front
		const bool shouldConstrict = isTrigConvexOnSide<side>(apex, selectSide<side>().front(), newVertex);
		if (shouldConstrict) {
			// Set first vertex to the innermost position in queue
			selectSide<side>() = std::deque<Vertex>({ newVertex });

			// Check if apex needs to be moved
			const bool shouldMoveApex = !selectSide<otherSide<side>>().empty()
				&& isTrigConvexOnSide<otherSide<side>>(apex, selectSide<side>().front(), selectSide<otherSide<side>>().front());
			return shouldMoveApex;
		}
		else {
			appendVertex<side>(newVertex);
			return false;
		}
	}

	template <Side side>
	double sideMaxCost() const
	{
		double acc = 0;
		auto prev  = apex;
		for (const auto& curr : selectSide<side>()) {
			acc += glm::distance(prev, curr);
			prev = curr;
		}
		return acc;
	}

	template <Side side>
	glm::dvec2 outermostProjection(const geom::edge2& funnelEnd, double& lengthAcc, bool& endReached, std::deque<Vertex>::const_iterator& curr) const
	{
		// With 1 vertices, solution is trivial
		const auto numVerticesOnSide = selectSide<side>().size();
		DK_ASSERT(numVerticesOnSide != 0);
		if (numVerticesOnSide == 1) 
		{
			endReached = true;
			curr = selectSide<side>().begin(); // same as end() - 1 if has 1 elements
			if (numVerticesOnSide > 0)
				lengthAcc += glm::distance(apex, selectSide<side>().front());
			return selectSide<side>().back();
		}
		lengthAcc += glm::distance(apex, selectSide<side>().front());
		// Move until line to projection doesn't cross funnel side
		for (curr = selectSide<side>().begin();;)
		{
			// End is reached
			if (*curr == selectSide<side>().back()) {
				endReached = true;
				return selectSide<side>().back();
			}
			const auto projection = geom::projectPoint(funnelEnd, *curr);
			const auto next       = curr + 1;
			if (!isTrigConvexOnSide<side>(*curr, projection, *next))
				return projection;
			lengthAcc += glm::distance(*curr, *next);
			curr = next;
		}
		std::unreachable();
	}

	double sidesMinCost() const
	{
		if (leftQueue.empty() || rightQueue.empty())
			return 0;

		const auto funnelEnd      = endPortal();
		const auto apexProjection = geom::projectPoint(funnelEnd, apex);
		// Check if end is visible from apex
		if (!isTrigConvexOnSide<Side::Left>(apex, apexProjection, selectSide<Side::Left>().front())
			&& !isTrigConvexOnSide<Side::Right>(apex, apexProjection, selectSide<Side::Right>().front())) 
		{
			drawPoint(apexProjection);
			drawEdge(dk::geom::edge2{ apex, apexProjection });
			return glm::distance(apex, apexProjection);
		}

		double     sumLeft   = 0;
		double     sumRight  = 0;

		// Find outermost vertex on both sides (projection is furthest from the funnels side's end vertex)
		bool leftEndReached  = false;
		auto leftCurr        = selectSide<Side::Left>().end();
		glm::dvec2 outermostLeftProjection  = outermostProjection<Side::Left >(funnelEnd, sumLeft, leftEndReached, leftCurr);
		// And on other side
		bool rightEndReached = false;
		auto rightCurr       = selectSide<Side::Right>().end();
		glm::dvec2 outermostRightProjection = outermostProjection<Side::Right>(funnelEnd, sumRight, rightEndReached, rightCurr);

		// Add distance to projection
		double leftDistance = 0;
		if (!leftEndReached) {
			leftDistance = glm::distance(*leftCurr, outermostLeftProjection);
			sumLeft += leftDistance;
		}
		double rightDistance = 0;
		if (!rightEndReached) {
			rightDistance = glm::distance(*rightCurr, outermostRightProjection);
			sumRight += rightDistance;
		}
		// Handle crossing projections
		const bool projectionsCross = !isTrigConvexOnSide<Side::Left>(apex, outermostLeftProjection, outermostRightProjection);
		if (projectionsCross) {
			if (leftDistance > rightDistance) {
				DK_ASSERT(rightCurr != rightQueue.end());
				drawPoint(outermostRightProjection);
				drawEdge(dk::geom::edge2{ *rightCurr, outermostRightProjection });
				return sumRight;
			}
			else {
				DK_ASSERT(leftCurr != leftQueue.end());
				drawPoint(outermostLeftProjection);
				drawEdge(dk::geom::edge2{ *leftCurr, outermostLeftProjection });
				return sumLeft;
			}
		}

		if (sumLeft < sumRight) {
			if (!leftEndReached) {
				drawPoint(outermostLeftProjection);
				drawEdge(dk::geom::edge2{ *leftCurr, outermostLeftProjection });
			}
		}
		else if (!rightEndReached) {
			drawPoint(outermostRightProjection);
			drawEdge(dk::geom::edge2{ *rightCurr, outermostRightProjection });
		}

		return std::min(sumLeft, sumRight);
	}

	double tailLength() const;

	void drawEdge(const geom::edge2& edge) const;
	void drawPoint(const glm::dvec2& point) const;
};

}
