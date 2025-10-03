#include <devkit/gfx/mesh.h>

dk::gfx::MeshMask::MeshMask(Mesh& _mesh, VertexFlags target, VertexFlags original)
	: mesh(_mesh)
	, vertices(mesh.vertices)
	, indices(mesh.indices)
	, material(mesh.material)
{
	unsigned targetbits = static_cast<unsigned>(target);
	unsigned originalbits = static_cast<unsigned>(original);
	for (unsigned i = 0u; i < sizeof(VertexFlags); ++i)
	{
		if (targetbits % 2) // i th bit is set in flags
		{
			if (!(originalbits % 2))
				throw std::runtime_error("target includes flag not present in original");
			m_mask |= (1u << i);
		}

		// shift bits
		targetbits >>= 1u;
		originalbits >>= 1u;
	}

	m_mask = ~0u;
}
