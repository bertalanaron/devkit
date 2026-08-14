#include <devkit/gfx/mesh.h>

dk::gfx::MeshMask::MeshMask(Mesh& _mesh, VertexFlags target, VertexFlags original)
	: mesh(_mesh)
	, vertices(mesh.vertices)
	, indices(mesh.indices)
	, material(mesh.material)
{
	unsigned attributeIndex = 0;
	unsigned targetBits = static_cast<unsigned>(target);
	unsigned originalBits = static_cast<unsigned>(original);
	for (unsigned i = 0u; i < sizeof(VertexFlags) * 8u; ++i)
	{
		const bool targetHasFlag = targetBits & (1u << i);
		const bool originalHasFlag = originalBits & (1u << i);

		if (targetHasFlag && !originalHasFlag)
			throw std::runtime_error("target includes flag not present in original");

		if (originalHasFlag)
		{
			if (targetHasFlag)
				m_mask |= (1u << attributeIndex);
			++attributeIndex;
		}
	}
}
