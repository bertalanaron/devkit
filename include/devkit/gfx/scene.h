#pragma once
#include <devkit/gfx/mesh.h>
#include <devkit/common/filesystem_hierarchy_node.h>

namespace dk::gfx {

class Scene {
public:
	struct SceneNode 
		: public common::FSHierarchyNode<SceneNode> 
	{
		SceneNode(SceneNode&&)            = default;
		SceneNode& operator=(SceneNode&&) = default;
		SceneNode(Scene& scene, std::vector<unsigned>&& meshIndices)
			: m_scene(&scene)
			, m_meshIndices(std::move(meshIndices))
		{ }

		std::filesystem::path name;
		glm::mat4             transform = glm::identity<glm::mat4>();

		auto& operator[](unsigned index)
		{ return m_scene->m_meshFactories.at(m_meshIndices.at(index)); }

		auto meshes()
		{
			return m_meshIndices 
				| std::ranges::views::transform([&](unsigned index) {
					return m_scene->m_meshFactories.at(m_meshIndices.at(index))();
				});
		}

		auto meshes(VertexFlags flags)
		{
			return m_meshIndices 
				| std::ranges::views::transform([&](unsigned index) {
					return m_scene->m_meshFactories.at(m_meshIndices.at(index))(flags);
				});
		}

	private:
		Scene*                m_scene;
		std::vector<unsigned> m_meshIndices;

		friend class common::FSHierarchyNode<SceneNode>;
		// Temp:
		friend class Scene;
	};

public:
	auto meshes()
	{
		return m_meshFactories
			| std::ranges::views::transform([](auto& factory) -> MeshMask& { 
				return factory(); 
			});
	}

	auto meshes(VertexFlags flags)
	{
		return m_meshFactories
			| std::ranges::views::transform([flags](auto& factory) -> MeshMask& { 
				return factory(flags); 
			});
	}

	auto& operator[](const std::filesystem::path& path)
	{
		auto node = m_root->resolveRelativeNode(path);
		if (!node)
			throw std::runtime_error("Invalid object path");
		// Temp fix: 
		node->m_scene = this;
		return *node;
	}

	//const auto& operator[](const std::filesystem::path& path) const
	//{
	//	auto node = m_root->resolveRelativeNode(path);
	//	if (!node)
	//		throw std::runtime_error("Invalid object path");
	//	return *node;
	//}

	Scene()                   = default;
	Scene(Scene&&)            = default;
	Scene& operator=(Scene&&) = default;

	Scene(const Scene&);
	Scene(const std::filesystem::path& path);

	static Scene load(const std::string& path)
	{ return Scene(path); }

	void update(const std::string& path)
	{ (*this) = std::move(Scene(path)); }

public:
	class MeshFactory {
	public:
		MeshFactory(MeshFactory&&) = default;
		MeshFactory(const MeshFactory&) = delete;
		MeshFactory(Mesh&& original, VertexFlags flags)
			: m_originalFlags(flags)
			, m_originalMesh(std::make_unique<Mesh>(original))
		{ 
			m_instances.emplace(flags, std::make_unique<MeshMask>(*m_originalMesh, flags, flags));
		}

		// @brief Get original mesh as a MeshMask
		MeshMask& operator()()
		{ return *m_instances.at(m_originalFlags); }

		MeshMask& operator()(VertexFlags target)
		{
			auto it = m_instances.find(target);
			if (it == m_instances.end())
				it = m_instances.emplace(target, std::make_unique<MeshMask>(*m_originalMesh, target, m_originalFlags)).first;
			return *it->second;
		}

	private:
		using MeshMaskLUT = std::unordered_map<VertexFlags, std::unique_ptr<MeshMask>>;

		const VertexFlags     m_originalFlags;
		std::unique_ptr<Mesh> m_originalMesh;
		MeshMaskLUT           m_instances;
	};

private:
	std::optional<SceneNode> m_root;
	std::vector<MeshFactory> m_meshFactories;

	friend class SceneNode;
};

} // dk::gfx
