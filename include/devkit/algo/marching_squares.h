#pragma once
#include <devkit/algo/geometry.h>

#include <devkit/algo/draw.h>

namespace dk::algo {

//struct CellInfo {
//	double vertexOffset = .5;  // 0 -> off, 1 -> off, 0..1 -> edge
//};

using MarchingSquaresCellInfo = std::pair<bool, double>;

template <typename GetCell, typename GetEdgeOffset>
	requires requires(GetCell getCell, GetEdgeOffset getEdgeOffset, int x, int y) {
		{ getCell(x, y) } -> std::same_as<bool>;
		{ getEdgeOffset(x, y, x, y) } -> std::same_as<double>;
	}
std::unordered_map<glm::dvec2, glm::dvec2> marchingSquaresConstructEdges(
	GetCell       getCell, 
	GetEdgeOffset getEdgeOffset,
	glm::ivec2    size, 
	glm::ivec2	  offset           = { 0, 0 },
	bool	      allowDiagonals   = false,
	bool          doTraceBoundary  = true,
	//double      offsetFromVertex = .5,
	glm::dvec2*   firstFoundVertex = nullptr)  // useful for finding the outer boundary instead of a hole
{
	std::unordered_map<glm::dvec2, glm::dvec2> result;

	// Lookup table for the Marching Squares algorithm
	static constexpr int s_marchingSquaresLookupAllowDiags[16][4] = {
		{ -1, -1, -1, -1 }, {  6,  1, -1, -1 }, {  0,  3, -1, -1 }, {  6,  3, -1, -1 },
		{  2,  5, -1, -1 }, {  6,  5,  2,  1 }, {  0,  5, -1, -1 }, {  6,  5, -1, -1 },
		{  4,  7, -1, -1 }, {  4,  1, -1, -1 }, {  0,  7,  4,  3 }, {  4,  3, -1, -1 },
		{  2,  7, -1, -1 }, {  2,  1, -1, -1 }, {  0,  7, -1, -1 }, { -1, -1, -1, -1 }
	};
	// When not allowing diagonals one region can result in multiple polygons.
	// Correct implementation should prevent most diagonals at the region identification step.
	// Use of this table is only needed because of regions whos cells are not only connected trhough diagonals. 
	static constexpr int s_marchingSquaresLookupDontAllowDiags[16][4] = {
		{ -1, -1, -1, -1 }, {  6,  1, -1, -1 }, {  0,  3, -1, -1 }, {  6,  3, -1, -1 },
		{  2,  5, -1, -1 }, {  2,  5,  6,  1 }, {  0,  5, -1, -1 }, {  6,  5, -1, -1 },
		{  4,  7, -1, -1 }, {  4,  1, -1, -1 }, {  4,  7,  0,  3 }, {  4,  3, -1, -1 },
		{  2,  7, -1, -1 }, {  2,  1, -1, -1 }, {  0,  7, -1, -1 }, { -1, -1, -1, -1 }
	};
	const auto& lookup = (allowDiagonals) 
		? s_marchingSquaresLookupAllowDiags
		: s_marchingSquaresLookupDontAllowDiags;

	constexpr static std::array<glm::dvec2, 4> borderOffsetLookup {
		glm::dvec2(-1, -1), glm::dvec2(0, .5), glm::dvec2(.5, 1), glm::dvec2(0, 1)
	};

	// Trace outer boundary square
	bool firstVertFound = false;
	if (doTraceBoundary)
	{
		glm::ivec2 direction{ 1, 0 };
		glm::ivec2 coord{ offset.x, offset.y };
		for (bool first = true;;first = false,coord += direction) {
			// Trace border
			if (glm::ivec2{ offset.x + size.x, offset.y          } == coord) direction = { 0, 1 };
			if (glm::ivec2{ offset.x + size.x, offset.y + size.y } == coord) direction = { -1, 0 };
			if (glm::ivec2{ offset.x, offset.y + size.y          } == coord) direction = { 0, -1 };
			if (glm::ivec2{ offset.x, offset.y                   } == coord && !first) break;

			auto next = coord + direction;

			// Get offset index
			int borderOffsetIndex = 0;
			if (getCell(coord.x, coord.y)) borderOffsetIndex |= 1;
			if (getCell(next.x, next.y))   borderOffsetIndex |= 2;
			// Lookup offset
			glm::dvec2 borderOffset = borderOffsetLookup[borderOffsetIndex];
			if (borderOffset.x < 0)
				continue;

			// Get exact offset through callback
			if (borderOffset.x == .5)
				borderOffset.x = 1 - getEdgeOffset(next.x, next.y, coord.x, coord.y);
			if (borderOffset.y == .5)
				borderOffset.y = getEdgeOffset(coord.x, coord.y, next.x, next.y);

			const glm::dvec2 vertex1 = glm::dvec2(coord.x + .5, coord.y + .5) + glm::dvec2(next - coord) * borderOffset.y;
			const glm::dvec2 vertex2 = glm::dvec2(coord.x + .5, coord.y + .5) + glm::dvec2(next - coord) * borderOffset.x;

			if (!firstVertFound) {
				firstVertFound = true;
				if (firstFoundVertex != nullptr)
					*firstFoundVertex = glm::dvec2(vertex1);
			}
			result.insert({ glm::dvec2(vertex1), glm::dvec2(vertex2) });
		}
	}

	static std::array<MarchingSquaresCellInfo, 4> edgeVertices;
	constexpr static std::array<std::array<glm::ivec2, 2>, 8> edgeRelativeCoordsLookup {
		std::array{glm::ivec2(1, 0), glm::ivec2(0, 0)}, std::array{glm::ivec2(0, 0), glm::ivec2(1, 0)},
		std::array{glm::ivec2(1, 1), glm::ivec2(1, 0)}, std::array{glm::ivec2(1, 0), glm::ivec2(1, 1)},
		std::array{glm::ivec2(0, 1), glm::ivec2(1, 1)}, std::array{glm::ivec2(1, 1), glm::ivec2(0, 1)},
		std::array{glm::ivec2(0, 0), glm::ivec2(0, 1)}, std::array{glm::ivec2(0, 1), glm::ivec2(0, 0)}
	};

	// Construct edges using marching squares
	for (int y = offset.y; y < size.y + offset.y; ++y) for (int x = offset.x; x < size.x + offset.x; ++x) {
		int index = 0;
		if (getCell(x + 0, y + 0)) index |= 1;
		if (getCell(x + 1, y + 0)) index |= 2;
		if (getCell(x + 1, y + 1)) index |= 4;
		if (getCell(x + 0, y + 1)) index |= 8;

		for (int i = 0; i < 2; ++i) {
			if (lookup[index][i * 2] == -1) 
				continue;

			const auto& edgeRelativeCoords1 = edgeRelativeCoordsLookup[lookup[index][i * 2 + 0]];
			const auto& edgeRelativeCoords2 = edgeRelativeCoordsLookup[lookup[index][i * 2 + 1]];

			const auto offset1 = getEdgeOffset(
				x + edgeRelativeCoords1[0].x, y + edgeRelativeCoords1[0].y,
				x + edgeRelativeCoords1[1].x, y + edgeRelativeCoords1[1].y);
			const auto offset2 = getEdgeOffset(
				x + edgeRelativeCoords2[0].x, y + edgeRelativeCoords2[0].y,
				x + edgeRelativeCoords2[1].x, y + edgeRelativeCoords2[1].y);

			const auto vertex1 = glm::dvec2(x + .5 + edgeRelativeCoords1[0].x, y + .5 + edgeRelativeCoords1[0].y) + offset1 * glm::dvec2(edgeRelativeCoords1[1] - edgeRelativeCoords1[0]);
			const auto vertex2 = glm::dvec2(x + .5 + edgeRelativeCoords2[0].x, y + .5 + edgeRelativeCoords2[0].y) + offset2 * glm::dvec2(edgeRelativeCoords2[1] - edgeRelativeCoords2[0]);

			if (!firstVertFound) {
				firstVertFound = true;
				if (firstFoundVertex != nullptr)
					*firstFoundVertex = vertex1;
			}
			result.insert({ vertex1, vertex2 });
		}
	}

	return result;
}

template <typename GetCell, typename GetEdgeOffset>
	requires requires(GetCell getCell, GetEdgeOffset getEdgeOffset, int x, int y) {
		{ getCell(x, y) } -> std::same_as<bool>;
		{ getEdgeOffset(x, y, x, y) } -> std::same_as<double>;
}
geom::polygon2 marchingSquaresConstructPolygon(
	GetCell       getCell, 
	GetEdgeOffset getEdgeOffset,
	glm::ivec2    size, 
	glm::ivec2	  offset           = { 0, 0 },
	bool	      allowDiagonals   = false,
	bool          doTraceBoundary  = true
	//double      offsetFromVertex = .5
)  
{
	// Find edges using marching squares
	glm::dvec2 firstVertexFound;
	const auto edges = algo::marchingSquaresConstructEdges(getCell, getEdgeOffset, size, offset, allowDiagonals, doTraceBoundary, &firstVertexFound);

	// Construct polygon from edges
	geom::polygon2 polygon;
	if (edges.empty())
		return polygon;

	// Remove non corner vertices from edge loop
	constexpr static auto connectEdges = [](
		const std::unordered_map<glm::dvec2, glm::dvec2>& input, 
		const glm::dvec2&                                 first, 
		std::unordered_set<glm::dvec2>&                   visited, 
		std::vector<glm::dvec2>&                          output) 
	{
		auto it = input.find(first);
		glm::dvec2 prev = it->first;
		for (it = input.find(it->second); 
			!visited.contains(it->first); 
			prev = it->first, it = input.find(it->second)) 
		{
			visited.insert(it->first);

			glm::dvec2 d1 = glm::normalize(it->first - prev);
			glm::dvec2 d2 = glm::normalize(it->second - it->first);

			if (glm::dot(d1, d2) < 0.99)
				output.push_back(it->first);
		}
	};

	// Trace polygon boundary
	std::unordered_set<glm::dvec2> visitedEdges{};
	connectEdges(edges, firstVertexFound, visitedEdges, polygon.vertices);
	
	// Identify and trace holes inside the region and store them in polygon.holes
	for (auto& edge : edges) {
		if (visitedEdges.contains(edge.first))
			continue;

		std::vector<glm::dvec2> hole{};
		connectEdges(edges, edge.first, visitedEdges, hole);

		polygon.holes.push_back(hole);
	}

	return polygon;
}

} // dk::algo
