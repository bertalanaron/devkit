#pragma once
#include <devkit/algo/geometry.h>

namespace dk::algo {

template <typename GetCell, typename SetIsland>
	requires requires(GetCell getCell, int x, int y, SetIsland setIsland, glm::ivec2 coords, std::optional<glm::ivec2> from, int island) {
		{ getCell(x, y, x, y /*, from*/) } -> std::same_as<bool>;
		{ setIsland(x, y, island) };
	}
int floodFillGrid(
	GetCell           getCell, 
	SetIsland         setIsland, 
	const glm::ivec2& size, 
	const glm::ivec2& offset         = { 0, 0 }, 
	bool              allowDiagonals = false)
{
	int count = 0;
	std::vector<bool> visited(size.x * size.y);

	for (int y = 0; y < size.y; ++y) for (int x = 0; x < size.x; ++x) {
		// Continue if empty
		if (!getCell(x + offset.x, y + offset.y, x + offset.x, y + offset.y /*, std::nullopt*/) || visited.at(y * size.x + x))
		//if (visited.at(y * size.x + x))
			continue;
		++count;

		// Setup queue for neighbours
		std::queue<glm::ivec2> q;
		q.push({ x, y });

		// While the visited cells have unvisited neighbours add them to the region
		while (!q.empty()) {
			glm::ivec2 c = q.front(); q.pop();
			setIsland(c.x + offset.x, c.y + offset.y, count);

			// Get neighbours based on whether diagonals are allowed
			static const std::vector<glm::ivec2> directionsAllowDiags = { 
				{1,0}, {-1,0}, {0,1}, {0,-1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
			static const std::vector<glm::ivec2> directionsDontAllowDiags = { {1,0}, {-1,0}, {0,1}, {0,-1} };
			const std::vector<glm::ivec2>& directinos = allowDiagonals ? directionsAllowDiags : directionsDontAllowDiags;

			// Check neighbours
			for (const glm::ivec2& d : directinos) {
				int nx = c.x + d.x;
				int ny = c.y + d.y;

				if (nx >= 0 && ny >= 0 && nx < size.x && ny < size.y
					&& getCell(nx + offset.x, ny + offset.y, c.x + offset.x, c.y + offset.y/*, c*/) && !visited.at(ny * size.x + nx))
				{
					q.push({ nx, ny });
					visited.at(ny * size.x + nx) = true;
				}
			}
		}
	}

	return count;
}

} // dk::algo
