#include <devkit/algo/navmesh.h>

#include <devkit/algo/draw.h>

void dk::algo::NavmeshChunk::pushPolygon(const dk::geom::polygon2& polygon, NavmeshDecomposition decomp)
{
	// Split polygons into convex parts
	const auto convexParts = [=]{
		switch (decomp) {
		case NavmeshDecomposition::CDT:          return polygon.triangulate();
		case NavmeshDecomposition::ConvexDecomp: return polygon.convexDecomp();
		default: throw std::runtime_error("Incorrect decomposition value");
		}
	}();
	auto out = dk::dbg::store_or<dk::gfx::VertexSink*, "navmesh_poly_out">(nullptr);
	std::vector<glm::vec4> colors { DK_COLOR(0x5d0203ff), DK_COLOR(0xbc3918ff), DK_COLOR(0xfe9c44ff), DK_COLOR(0xf9f387ff), DK_COLOR(0x765d5dff) };
	for (const auto& part : convexParts) {
		int height = dk::dbg::store_or<int, "current_height">(1);
		const auto& color = colors.at(height % colors.size());
		*out << dk::gfx::draw(part, color, color, dk::geom::plane::Y() + height + .001, -dk::geom::axis::X);
	}

	// Move polygons to output and update index
	m_polygons.reserve(m_polygons.size() + convexParts.size());
	for (auto& part : convexParts) {
		m_polygons.push_back(part);

		const auto& vertices = m_polygons.back().vertices;
		for (int i = 0; i < vertices.size() + 1; ++i) {
			dk::geom::edge2 edge{ vertices.at((i + 1) % vertices.size()), vertices.at(i % vertices.size()) };
			m_polygonLookup.emplace(edge, m_polygons.size() - 1);
		}
	}
}

void dk::algo::NavmeshChunk::clearPolygons()
{
	m_polygons.clear();
	m_polygonLookup.clear();
	m_nodeLevels.clear();
}

const dk::geom::polygon2* dk::algo::NavmeshChunk::polygonOfEdge(const dk::geom::edge2& edge) const
{
	auto it = m_polygonLookup.find(edge);
	if (it == m_polygonLookup.end())
		return nullptr;
	return &m_polygons.at(it->second);
}

dk::geom::edge2 dk::algo::NavmeshChunk::nearestEdge(const glm::dvec2& point) const
{
	dk::geom::edge2 minEdge{};
	double minDistance = std::numeric_limits<double>::infinity();
	for (auto& polygon : m_polygons) {
		//if (!polygon.isPointInside(point))
		//	continue;

		for (int i = 0; i < polygon.vertices.size() + 1; ++i) {
			dk::geom::edge2 edge{ polygon.vertices.at(i % polygon.vertices.size()), polygon.vertices.at((i + 1) % polygon.vertices.size()) };
			double distance = dk::geom::distance(edge, point);

			if (distance < minDistance) {
				minDistance = distance;
				minEdge = edge;
			}
		}
	}
	return minEdge;
}

const dk::geom::polygon2* dk::algo::NavmeshChunk::polygonAtPoint(const glm::dvec2& point) const
{
	//for (const auto& polygon : m_polygons) {
	//	if (polygon.isPointInside(point)) {
	//		return &polygon;
	//	}
	//}
	return nullptr;
}

void dk::algo::NavmeshChunk::calculateNodeLevels(Navmesh& navmesh)
{
	for (const auto& node : m_polygons)
	{
		int neighbourCount = 0;
		for (int i = 0; i < node.vertices.size(); ++i)
		{
			if (neighbourCount > 1)
				break;
			const auto edge = dk::geom::edge2{ node.vertices.at(i), node.vertices.at((i + 1) % node.vertices.size()) };
			if (navmesh.polygonOfEdge(edge) != nullptr)
				neighbourCount += 1;
		}
		m_nodeLevels[&node] = neighbourCount;
	}
}

void dk::algo::NavmeshChunk::promoteLevel3Nodes(Navmesh& navmesh)
{
	for (const auto& node : m_polygons)
	{
		int lvl2NeighbourCount = 0;
		for (int i = 0; i < node.vertices.size(); ++i)
		{
			if (lvl2NeighbourCount > 2)
				break;
			const auto edge      = dk::geom::edge2{ node.vertices.at(i), node.vertices.at((i + 1) % node.vertices.size()) };
			if (navmesh.nodeLevelThroughEdge(edge) >= 2)
				lvl2NeighbourCount += 1;
		}
		if (lvl2NeighbourCount > 2)
			m_nodeLevels[&node] = 3;

		if (lvl2NeighbourCount > 2)
		{
			auto out = dk::dbg::store_or<dk::gfx::VertexSink*, "navmesh_poly_out">(nullptr);
			*out << dk::gfx::draw(node.centroid(), dk::colors::orange, dk::geom::plane::Y() + 1, -dk::geom::axis::X);
		}
	}
}

int dk::algo::NavmeshChunk::nodeLevelThroughEdge(const geom::edge2& edge) const
{
	const auto node = polygonOfEdge(edge);
	if (node == nullptr)
		return 0;
	return m_nodeLevels.at(node);
}

dk::algo::NavmeshGenerator::NavmeshGenerator(int threadCount)
	: m_isConcurrent(threadCount > 0)
	, m_threadCount(threadCount)
{
	// Start worker threads
	if (m_isConcurrent) {
		for (int i = 0; i < threadCount; ++i) {
			m_workers.push_back(std::jthread([this, i](std::stop_token stopToken) { runWorker(stopToken); }));
		}
	}
}

void dk::algo::NavmeshGenerator::build(glm::ivec2 sizeInChunks, std::vector<NavmeshChunk>& chunks, Navmesh& navmesh) {
	bool eitherChanged = false;
	if (m_isConcurrent) {
		for (int i = 0; i < 4; ++i) {
			for (int y = i / 2; y < sizeInChunks.y; y += 2) 
				for (int x = i % 2; x < sizeInChunks.x; x += 2) 
				{
					const glm::ivec2 coord(x, y);
					const int index = y * sizeInChunks.x + x;

					if (chunkUpdated(coord)) {
						m_jobs.push(GenerateTask{ 
						.chunk = chunks[index],
						.coord = coord });
						eitherChanged = true;
					}
				}

			m_jobs.wait();
		}
	}
	else {
		for (int y = 0; y < sizeInChunks.y; ++y)
			for (int x = 0; x < sizeInChunks.x; ++x)
			{
				const glm::ivec2 coord(x, y);
				const int index = y * sizeInChunks.x + x;
				if (chunkUpdated(coord)) {
					generateChunk(chunks[index], coord);
					eitherChanged = true;
				}
			}
	}
	if (eitherChanged) {
		calculateNodeLevels(sizeInChunks, chunks, navmesh);
		promoteLevel3Nodes(sizeInChunks, chunks, navmesh);
	}
	buildDone();
}

void dk::algo::NavmeshGenerator::calculateNodeLevels(glm::ivec2 sizeInChunks, std::vector<NavmeshChunk>& chunks, Navmesh& navmesh)
{
	if (m_isConcurrent) {
		for (int y = 0; y < sizeInChunks.y; ++y)
			for (int x = 0; x < sizeInChunks.x; ++x) 
			{
				const glm::ivec2 coord(x, y);
				const int index = y * sizeInChunks.x + x;

				if (chunkUpdated(coord))
					m_jobs.push(CalcNodeLevelsTask{ 
					.chunk = chunks[index],
					.navmesh = navmesh });
			}
		m_jobs.wait();
	}
	else {
		for (int y = 0; y < sizeInChunks.y; ++y)
			for (int x = 0; x < sizeInChunks.x; ++x)
			{
				const glm::ivec2 coord(x, y);
				const int index = y * sizeInChunks.x + x;
				if (chunkUpdated(coord))
					chunks[index].calculateNodeLevels(navmesh);
			}
	}
}

void dk::algo::NavmeshGenerator::promoteLevel3Nodes(glm::ivec2 sizeInChunks, std::vector<NavmeshChunk>& chunks, Navmesh& navmesh)
{
	if (m_isConcurrent) {
		for (int i = 0; i < 4; ++i) {
			for (int y = i / 2; y < sizeInChunks.y; y += 2) 
				for (int x = i % 2; x < sizeInChunks.x; x += 2) 
				{
					const glm::ivec2 coord(x, y);
					const int index = y * sizeInChunks.x + x;
					m_jobs.push(PromoteLevel3NodesTask{ 
					.chunk = chunks[index],
					.navmesh = navmesh });
				}

			m_jobs.wait();
		}
	}
	else {
		for (int y = 0; y < sizeInChunks.y; ++y)
			for (int x = 0; x < sizeInChunks.x; ++x)
			{
				const glm::ivec2 coord(x, y);
				const int index = y * sizeInChunks.x + x;
				chunks[index].promoteLevel3Nodes(navmesh);
			}
	}
}

void dk::algo::NavmeshGenerator::runWorker(std::stop_token stopToken) {
	while (true) {
		auto maybeJob = m_jobs.pop(stopToken);
		if (maybeJob.has_value()) {
			const auto& job = maybeJob.value();
			std::visit(common::overload{
				[this](const GenerateTask& task) { 
					generateChunk(task.chunk, task.coord); 
				},
				[this](const CalcNodeLevelsTask& task) { 
					task.chunk.calculateNodeLevels(task.navmesh);
				},
				[this](const PromoteLevel3NodesTask& task) { 
					task.chunk.promoteLevel3Nodes(task.navmesh);
				}
			}, job);
			m_jobs.done();
		}
		else if (dk::concurrency::Signal::Shutdown == maybeJob.error())
			break;
	}
}

dk::algo::Navmesh::Navmesh(glm::ivec2 sizeInChunks, glm::ivec2 chunkSize)
	: m_sizeInChunks(sizeInChunks)
	, m_chunkSize(chunkSize)
{
	m_chunks.resize(m_sizeInChunks.x * m_sizeInChunks.y);
}

void dk::algo::Navmesh::build(NavmeshGenerator& generator) {
	generator.build(m_sizeInChunks, m_chunks, *this);
}

const dk::algo::NavmeshChunk* dk::algo::Navmesh::chunkOfEdge(const dk::geom::edge2& edge) const
{
	const auto& start = edge.at(0), & end = edge.at(1);
	glm::dvec2 midpoint = (start + end) / 2.0;

	glm::dvec2 chunkCoord;
	if (std::modf((midpoint.x - 0.5) / (double)m_chunkSize.x, &chunkCoord.x) == 0.0
		&& start.y > end.y)
		chunkCoord.x = std::max(chunkCoord.x - 1, 0.0);
	if (std::modf((midpoint.y - 0.5) / (double)m_chunkSize.y, &chunkCoord.y) == 0.0
		&& start.x < end.x)
		chunkCoord.y = std::max(chunkCoord.y - 1, 0.0);

	auto index = chunkCoord.y * m_sizeInChunks.x + chunkCoord.x;
	if (index < 0 || index >= m_chunks.size())
		return nullptr;

	return &m_chunks.at(index);
}

const dk::algo::NavmeshChunk& dk::algo::Navmesh::chunkAtPoint(const glm::dvec2& point) const
{
	glm::dvec2 chunkCoord;
	std::modf((point.x - 0.5) / (double)m_chunkSize.x, &chunkCoord.x);
	std::modf((point.y - 0.5) / (double)m_chunkSize.y, &chunkCoord.y);

	// Clamp to valid range
	chunkCoord.x = std::clamp(chunkCoord.x, 0.0, (double)(m_sizeInChunks.x - 1));
	chunkCoord.y = std::clamp(chunkCoord.y, 0.0, (double)(m_sizeInChunks.y - 1));

	return m_chunks.at(static_cast<std::size_t>(chunkCoord.y) * m_sizeInChunks.x + static_cast<std::size_t>(chunkCoord.x));
}
