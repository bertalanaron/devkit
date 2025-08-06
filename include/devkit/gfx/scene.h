#pragma once
#include <devkit/gfx/mesh.h>

namespace dk::gfx {

class Scene {
private:
	struct Model {
	public:
		Model(Scene* scene, const glm::mat4& transform)
			: m_scene(scene)
			, m_transfrom(transform)
		{ }

		auto meshes()
		{
			using namespace std::ranges;

			return m_meshIndices 
				| views::transform([this](const auto& index) {
					return std::ref(m_scene->m_meshes.at(index));
				});
		}

		const glm::mat4& transform() const
		{ return m_transfrom; }

	private:
		Scene*           m_scene;
		glm::mat4        m_transfrom;
		std::vector<int> m_meshIndices;

		friend class Scene;
	};

public:
	static Scene load(const std::string& path);

	std::vector<Mesh>& meshes()
	{ return m_meshes; }

	Model& operator[](const std::string& name)
	{ return m_models.at(name); }

	const Model& operator[](const std::string& name) const
	{ return m_models.at(name); }

	Scene(Scene&& other)
		: m_directory(std::move(other.m_directory))
		, m_meshes(std::move(other.m_meshes))
		, m_models(std::move(other.m_models))
	{ 
		for (auto& model : m_models)
			model.second.m_scene = this;
	}

private:
	std::string                            m_directory;
	std::vector<Mesh>                      m_meshes;
	std::unordered_map<std::string, Model> m_models;

	void processNode(void* node, const void* scene, const glm::mat4& parentTransform);
	Mesh processMesh(void* mesh, const void* scene);

	Scene() = default;
};

} // dk::gfx
