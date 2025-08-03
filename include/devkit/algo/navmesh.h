#pragma once
#include <devkit/common/utils.h>
#include <devkit/algo/geometry.h>
#include <devkit/algo/navmesh_pathfinding.h>
#include <devkit/algo/task_queue.h>

namespace dk::algo {

enum class NavmeshDecomposition { CDT, ConvexDecomp };

struct Navmesh;

struct NavmeshChunk {
public:
	void pushPolygon(const geom::polygon2& polygon, NavmeshDecomposition decomp);

	void clearPolygons();

	const geom::polygon2* polygonOfEdge(const geom::edge2& edge) const;

	geom::edge2 nearestEdge(const glm::dvec2& point) const;

	const geom::polygon2* polygonAtPoint(const glm::dvec2& point) const;

	void calculateNodeLevels(Navmesh& navmesh);

	void promoteLevel3Nodes(Navmesh& navmesh);

	int nodeLevelThroughEdge(const geom::edge2& edge) const;

private:
	std::vector<geom::polygon2>                    m_polygons;
	std::unordered_map<geom::edge2, int>           m_polygonLookup{};
	std::unordered_map<const geom::polygon2*, int> m_nodeLevels;
};

struct NavmeshGenerator {
public:
	NavmeshGenerator(int threadCount = 0);
 
	void build(glm::ivec2 sizeInChunks, std::vector<NavmeshChunk>& chunks, Navmesh& navmesh);

private:
	struct GenerateTask {
		NavmeshChunk& chunk;
		glm::ivec2    coord; 
	};

	struct CalcNodeLevelsTask {
		NavmeshChunk& chunk;
		Navmesh&      navmesh;
	};

	struct PromoteLevel3NodesTask {
		NavmeshChunk& chunk;
		Navmesh&      navmesh;
	};

	using Task = std::variant<GenerateTask, CalcNodeLevelsTask, PromoteLevel3NodesTask>;

	//struct ChunkGenerationJob {
	//	NavmeshChunk& chunk;
	//	glm::ivec2    coord; 
	//};

	using TaskQueue = dk::concurrency::TaskQueue<Task>;

	const bool                m_isConcurrent; ///< Indicates whether generation is multi-threaded.
	const int                 m_threadCount;  ///< Number of worker threads.
	std::vector<std::jthread> m_workers;      ///< Worker threads for parallel processing.
	TaskQueue                 m_jobs;         ///< Job queue for managing chunk generation tasks.

	void runWorker(std::stop_token stopToken);

	void calculateNodeLevels(glm::ivec2 sizeInChunks, std::vector<NavmeshChunk>& chunks, Navmesh& navmesh);

	void promoteLevel3Nodes(glm::ivec2 sizeInChunks, std::vector<NavmeshChunk>& chunks, Navmesh& navmesh);

protected:
	virtual bool chunkUpdated(const glm::ivec2& chunkCoord) const = 0;

	virtual void generateChunk(NavmeshChunk& chunk, const glm::ivec2& chunkCoord) = 0;

	virtual void buildDone() { }
};

/**
* @class Navmesh
* @brief Represents a navigation mesh composed of multiple navigation mesh chunks.
*
* This class manages a grid-based navigation mesh, allowing efficient lookups of polygons and edges
* for pathfinding and navigation. It extends NavmeshNavigator to provide higher-level navigation functionalities.
*/
struct Navmesh : public NavmeshPathfinder {
public:
	Navmesh(glm::ivec2 sizeInChunks, glm::ivec2 chunkSize);

	/**
	* @brief Builds the navigation mesh using the given generator.
	* 
	* This function uses a NavmeshGenerator to construct the navigation mesh
	* by generating and storing chunks in a structured grid layout.
	* 
	* @param generator The NavmeshGenerator used to create the navigation mesh.
	*/
	void build(NavmeshGenerator& generator);

private:
	std::vector<NavmeshChunk> m_chunks;       ///< Storage for navigation mesh chunks.
	glm::ivec2                m_sizeInChunks; ///< Size of the navigation mesh in chunk units.
	glm::ivec2                m_chunkSize;    ///< Size of each chunk in world units.

	/**
	* @brief Retrieves the navigation mesh chunk that contains the given edge.
	* 
	* @param edge The edge to find the corresponding chunk for.
	* @return Reference to the corresponding NavmeshChunk.
	*/
	const NavmeshChunk* chunkOfEdge(const geom::edge2& edge) const;

	/**
	* @brief Retrieves the navigation mesh chunk at the given world position.
	* 
	* @param point The world-space position.
	* @return Reference to the corresponding NavmeshChunk.
	*/
	const NavmeshChunk& chunkAtPoint(const glm::dvec2& point) const;

private:
public:
	const geom::polygon2* polygonOfEdge(const geom::edge2& edge) const {
		auto chunk = chunkOfEdge(edge);
		if (!chunk)
			return nullptr;
		return chunk->polygonOfEdge(edge);
	}

	int nodeLevelThroughEdge(const geom::edge2& edge) const
	{
		auto chunk = chunkOfEdge(edge);
		if (!chunk)
			return 0;
		return chunk->nodeLevelThroughEdge(edge);
	}

	geom::edge2 nearestEdge(const glm::dvec2& point) const {
		return chunkAtPoint(point).nearestEdge(point);
	}

	const geom::polygon2* polygonAtPoint(const glm::dvec2& point) const {
		return chunkAtPoint(point).polygonAtPoint(point);
	}

	bool isVertexOnBoundary(const glm::dvec2& vertex) const {
		// Return true if vertex is on the edge of a chunk
		glm::dvec2 chunkCoord;
		if (std::modf((vertex.x - 0.5) / (double)m_chunkSize.x, &chunkCoord.x) == 0.0
			&& std::modf((vertex.y - 0.5) / (double)m_chunkSize.y, &chunkCoord.y) == 0.0)
			return false;
		return true;
	}

	friend class NavmeshNavigator;
};

} // dk::algo
