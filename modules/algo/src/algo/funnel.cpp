#include <devkit/algo/funnel.h>

#include <devkit/algo/draw.h>

void dk::algo::Funnel::evaluateContainedPaths()
{
	// Calculate max cost
	m_maxCost = tailLength() + std::max(sideMaxCost<Side::Left>(), sideMaxCost<Side::Right>());
	m_minCost = tailLength() + sidesMinCost();
}

double dk::algo::Funnel::tailLength() const
{
	double acc = 0;
	auto prev  = apex;
	for (const auto& curr : tail) {
		acc += glm::distance(prev, curr);
		prev = curr;
	}
	return acc;
}

void dk::algo::Funnel::drawEdge(const geom::edge2& edge) const
{
	auto& dbg = dk::dbg::store<dk::gfx::VertexSink*, "funnel_dbg">();
	(*dbg) << dk::gfx::draw(edge, dk::colors::red, dk::geom::plane::Y() + 0.1, -dk::geom::axis::X);
}

void dk::algo::Funnel::drawPoint(const glm::dvec2& point) const
{
	auto& dbg = dk::dbg::store<dk::gfx::VertexSink*, "funnel_dbg">();
	(*dbg) << dk::gfx::draw(point, dk::colors::red, dk::geom::plane::Y() + 0.1, -dk::geom::axis::X);
}
