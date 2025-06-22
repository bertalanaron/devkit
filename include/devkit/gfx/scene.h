#pragma once
#include <devkit/gfx/mesh.h>

namespace dk::gfx {

class Scene {
public:
	static Scene load(const std::string& path);

	std::vector<Mesh>& meshes()
	{ return m_meshes; }

private:
	std::string       m_directory;
	std::vector<Mesh> m_meshes;

	void processNode(void* node, const void* scene);
	Mesh processMesh(void* mesh, const void* scene);
};

} // dk::gfx
