#pragma once
#include <devkit/common/utils.h>

namespace dk::algo {

enum class GridChunkLayout { InOrder, Grouped };

template <GridChunkLayout layout, bool doBoundsCheck = true>
class GridChunkAccessor {
public:
	GridChunkAccessor(const glm::ivec2* chunkSize, const glm::ivec2* gridSize)
		: m_chunkSize(chunkSize)
		, m_gridSize(gridSize)
	{  }

	glm::ivec2 sizeInChunks() const
	{
		return glm::ivec2(glm::ceil((double)m_gridSize->x / (double)m_chunkSize->x), glm::ceil((double)m_gridSize->y / (double)m_chunkSize->y));
	}

	bool isInbounds(const glm::ivec2& cellCoords) const
	{
		return 0 <= cellCoords.x && cellCoords.x < m_gridSize->x
			&& 0 <= cellCoords.y && cellCoords.y < m_gridSize->y;
	}

	glm::ivec2 chunkCoordsOf(const glm::ivec2& cellCoords) const
	{
		verifyCellCoords(cellCoords);
		return glm::ivec2(cellCoords.x / m_chunkSize->x, cellCoords.y / m_chunkSize->y);
	}

	glm::ivec2 chunkCoordsOf(int index) const
	{
		verifyCellIndex(index);
		return chunkCoordsOf(coordsOf(index));
	}

	glm::ivec2 subChunkCoords(const glm::ivec2& cellCoords) const
	{
		verifyCellCoords(cellCoords);
		return glm::ivec2(cellCoords.x % m_chunkSize->x, cellCoords.y % m_chunkSize->y);
	}

	int indexOf(const glm::ivec2& cellCoords) const
	{
		verifyCellCoords(cellCoords);
		if constexpr (layout == GridChunkLayout::InOrder) 
		{
			return cellCoords.y * m_gridSize->x + cellCoords.x;
		}
		if constexpr (layout == GridChunkLayout::Grouped)
		{
			 const auto chunkCoords = chunkCoordsOf(cellCoords);
			 const int chunkBegin   = (chunkCoords.y * sizeInChunks().x + chunkCoords.x) * (m_chunkSize->x * m_chunkSize->y);
			 const auto subChunk    = subChunkCoords(cellCoords);
			 return chunkBegin + subChunk.y * m_chunkSize->x + subChunk.x;
		}
	}

	glm::ivec2 coordsOf(int index) const
	{
		verifyCellIndex(index);
		if constexpr (layout == GridChunkLayout::InOrder) 
		{
			return glm::ivec2(index / m_gridSize->x, index % m_gridSize->x);
		}
		if constexpr (layout == GridChunkLayout::Grouped)
		{
			const auto chunkIndex     = index / (m_chunkSize->x * m_chunkSize->y);
			const auto subChunk       = index % (m_chunkSize->x * m_chunkSize->y);
			const auto chunkCoords    = glm::ivec2(chunkIndex % sizeInChunks().x, chunkIndex / sizeInChunks().x);
			const auto subChunkCoords = glm::ivec2(subChunk % m_chunkSize->x, subChunk / m_chunkSize->x);
			return chunkCoords * (*m_chunkSize) + subChunkCoords;
		}
	}


	glm::ivec2 nearestChunk(const glm::dvec2& point) const;
	glm::ivec2 nearestCell(const glm::dvec2& point) const;
	glm::ivec2 chunkActualSize(const glm::ivec2& chunkCoords) const;

private:
	const glm::ivec2* m_chunkSize = nullptr;
	const glm::ivec2* m_gridSize  = nullptr;

	void verifyCellCoords(const glm::ivec2& cellCoords) const
	{
		if constexpr (doBoundsCheck) 
		{
			if (!isInbounds(cellCoords))
				throw std::runtime_error("Cell coords out of bounds");
		}
	}

	void verifyCellIndex(int index) const
	{
		if constexpr (doBoundsCheck) 
		{
			if (0 > index || index >= m_gridSize->x * m_gridSize->y)
				throw std::runtime_error("Cell index out of bounds");
		}
	}
};

}
