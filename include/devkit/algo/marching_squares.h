#pragma once
#include <devkit/algo/geometry.h>

namespace dk::algo {

template <typename GetCell>
	requires requires(GetCell getCell, int x, int y) {
		{ getCell(x, y) } -> std::same_as<bool>;
	}
std::unordered_map<glm::dvec2, glm::dvec2> marchingSquaresConstructEdges(
	GetCell     getCell, 
	glm::ivec2  size, 
	glm::ivec2	offset           = { 0, 0 },
	bool	    allowDiagonals   = false,
	bool        doTraceBoundary  = true,
	double      offsetFromVertex = .5,
	glm::dvec2* firstFoundVertex = nullptr)  // useful for finding the outer boundary instead of a hole
{
	std::unordered_map<glm::dvec2, glm::dvec2> result;

	// Lookup table for the Marching Squares algorithm
	//static constexpr int s_marchingSquaresLookupAllowDiags[16][4] = {
	//	{ -1, -1, -1, -1 }, {  1,  0, -1, -1 }, {  2,  1, -1, -1 }, {  2,  0, -1, -1 },
	//	{  3,  2, -1, -1 }, {  3,  0,  1,  2 }, {  3,  1, -1, -1 }, {  3,  0, -1, -1 },
	//	{  0,  3, -1, -1 }, {  1,  3, -1, -1 }, {  0,  1,  2,  3 }, {  2,  3, -1, -1 },
	//	{  0,  2, -1, -1 }, {  1,  2, -1, -1 }, {  0,  1, -1, -1 }, { -1, -1, -1, -1 }
	//};
	static constexpr int s_marchingSquaresLookupAllowDiags[16][4] = {
		{ -1, -1, -1, -1 }, {  6,  1, -1, -1 }, {  0,  3, -1, -1 }, {  6,  3, -1, -1 },
		{  2,  5, -1, -1 }, {  6,  5,  2,  1 }, {  0,  5, -1, -1 }, {  6,  5, -1, -1 },
		{  4,  7, -1, -1 }, {  4,  1, -1, -1 }, {  0,  7,  4,  3 }, {  4,  3, -1, -1 },
		{  2,  7, -1, -1 }, {  2,  1, -1, -1 }, {  0,  7, -1, -1 }, { -1, -1, -1, -1 }
	};
	// When not allowing diagonals one region can result in multiple polygons.
	// Correct implementation should prevent most diagonals at the region identification step.
	// Use of this table is only needed because of regions whos cells are not only connected trhough diagonals. 
	//static constexpr int s_marchingSquaresLookupDontAllowDiags[16][4] = {
	//	{ -1, -1, -1, -1 }, {  1,  0, -1, -1 }, {  2,  1, -1, -1 }, {  2,  0, -1, -1 },
	//	{  3,  2, -1, -1 }, {  1,  0,  3,  2 }, {  3,  1, -1, -1 }, {  3,  0, -1, -1 },
	//	{  0,  3, -1, -1 }, {  1,  3, -1, -1 }, {  0,  3,  2,  1 }, {  2,  3, -1, -1 },
	//	{  0,  2, -1, -1 }, {  1,  2, -1, -1 }, {  0,  1, -1, -1 }, { -1, -1, -1, -1 }
	//};
	static constexpr int s_marchingSquaresLookupDontAllowDiags[16][4] = {
		{ -1, -1, -1, -1 }, {  6,  1, -1, -1 }, {  0,  3, -1, -1 }, {  6,  3, -1, -1 },
		{  2,  5, -1, -1 }, {  2,  5,  6,  1 }, {  0,  5, -1, -1 }, {  6,  5, -1, -1 },
		{  4,  7, -1, -1 }, {  4,  1, -1, -1 }, {  4,  7,  0,  3 }, {  4,  3, -1, -1 },
		{  2,  7, -1, -1 }, {  2,  1, -1, -1 }, {  0,  7, -1, -1 }, { -1, -1, -1, -1 }
	};
	const auto& lookup = (allowDiagonals) 
		? s_marchingSquaresLookupAllowDiags
		: s_marchingSquaresLookupDontAllowDiags;

	//static constexpr std::array<glm::dvec2, 4> vertexOffsets { 
	//	glm::dvec2{ .5f, 0.f }, glm::dvec2{ 0.f, .5f }, glm::dvec2{ .5f, 1.f }, glm::dvec2{ 1.f, .5f } };
	const std::array<glm::dvec2, 8> vertexOffsets {
		glm::dvec2(1 - offsetFromVertex, 0), glm::dvec2(offsetFromVertex, 0),
		glm::dvec2(1, 1 - offsetFromVertex), glm::dvec2(1, offsetFromVertex),
		glm::dvec2(offsetFromVertex, 1), glm::dvec2(1 - offsetFromVertex, 1),
		glm::dvec2(0, offsetFromVertex), glm::dvec2(0, 1 - offsetFromVertex),
	};

	// Trace outer boundary square
	bool firstVertFound = false;
	const std::array<glm::dvec2, 4> s_boundaryOffsetLookup {
		glm::dvec2{ -1, -1 }, glm::dvec2{ 0, offsetFromVertex }, glm::dvec2{ 1 - offsetFromVertex, 1 }, glm::dvec2{ 0, 1 } 
	};
	if (doTraceBoundary)
	{
		glm::ivec2 direction{ 1, 0 };
		glm::ivec2 coord{ offset.x, offset.y };
		for (bool first = true;;first = false,coord += direction) {
			if (glm::ivec2{ offset.x + size.x, offset.y          } == coord) direction = { 0, 1 };
			if (glm::ivec2{ offset.x + size.x, offset.y + size.y } == coord) direction = { -1, 0 };
			if (glm::ivec2{ offset.x, offset.y + size.y          } == coord) direction = { 0, -1 };
			if (glm::ivec2{ offset.x, offset.y                   } == coord && !first) break;

			int boundaryOffsetIndex = 0;
			auto next = coord + direction;
			if (getCell(coord.x, coord.y)) boundaryOffsetIndex |= 1;
			if (getCell(next.x, next.y))   boundaryOffsetIndex |= 2;

			const auto& factor = s_boundaryOffsetLookup.at(boundaryOffsetIndex);
			if (factor.x < 0)
				continue;

			glm::dvec2 pos = glm::dvec2(coord) + glm::dvec2{ 0.5, 0.5 };
			if (!firstVertFound) {
				firstVertFound = true;
				if (firstFoundVertex != nullptr)
					*firstFoundVertex = pos + glm::dvec2(direction) * factor.y;
			}
			result.insert({ pos + glm::dvec2(direction) * factor.y, pos + glm::dvec2(direction) * factor.x });
		}
	}

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

			if (!firstVertFound) {
				firstVertFound = true;
				if (firstFoundVertex != nullptr)
					*firstFoundVertex = glm::dvec2(x + .5, y + .5) + vertexOffsets[lookup[index][i * 2 + 0]];
			}
			result.insert({
				glm::dvec2(x + .5, y + .5) + vertexOffsets[lookup[index][i * 2 + 0]],
				glm::dvec2(x + .5, y + .5) + vertexOffsets[lookup[index][i * 2 + 1]]
			});
		}
	}

	return result;
}

template <typename GetCell>
	requires requires(GetCell getCell, int x, int y) {
		{ getCell(x, y) } -> std::same_as<bool>;
	}
geom::polygon2 marchingSquaresConstructPolygon(
	GetCell     getCell, 
	glm::ivec2  size, 
	glm::ivec2	offset           = { 0, 0 },
	bool	    allowDiagonals   = false,
	bool        doTraceBoundary  = true,
	double      offsetFromVertex = .5)  
{
	// Find edges using marching squares
	glm::dvec2 firstVertexFound;
	const auto edges = algo::marchingSquaresConstructEdges(getCell, size, offset, allowDiagonals, doTraceBoundary, offsetFromVertex, &firstVertexFound);

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
