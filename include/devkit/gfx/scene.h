#pragma once
#include <devkit/gfx/mesh.h>

namespace dk::gfx {

enum class TextureClass { Diffuse, Normal, Specular, Height, Emissive, Metalness, Roughness };

class Scene {
private:
	using path_t = std::filesystem::path;
	template <typename T>
	using opt_t  = std::optional<T>;
	template <typename K, typename T>
	using umap_t = std::unordered_map<K, T>;

public:
	class Node;

	class Object {
	public:
		using ObjectTextureCollection = std::unordered_map<TextureClass, std::vector<path_t>>;

		Node*                   parentNode = nullptr; 
		ObjectTextureCollection texturePaths;
		glm::mat4               transform  = glm::identity<glm::mat4>();

		const Mesh& mesh() const;

		Object() = default;
		Object(Node* _parentNode, void* aiNode, int index);

	private:
		int m_index;
		int m_meshIndex;
	};
	
	class Node {
	public:
		using ChildNodeCollection = std::unordered_map<path_t, std::optional<Node>>;

		Scene*              parentScene = nullptr;
		Node*               parentNode  = nullptr;
		ChildNodeCollection childNodes;
		path_t              path;
		path_t              name;
		std::vector<Object> objects;
		glm::mat4           transform   = glm::identity<glm::mat4>();

		Node() = default;
		Node(Scene* scene, const void* aiScene, void* aiNode, Node* parentNode = nullptr);
	};

public:
	path_t              path;
	std::optional<Node> rootNode;

	Node& operator[](const std::filesystem::path& path)
	{
		Node* node = findNode(path);
		if (!node)
			throw std::runtime_error("Invalid object path");
		return *node;
	}

	const Node& operator[](const path_t& path) const
	{
		const Node* node = findNode(path);
		if (!node)
			throw std::runtime_error("Invalid object path");
		return *node;
	}

	static Scene load(const std::string& path);

	Scene() = default;
	Scene(const Scene&);
	Scene(const path_t& path);

private:
	struct MeshStorage {
		VertexFlags                      originalFlags;
		umap_t<VertexFlags, opt_t<Mesh>> instances;
	};

	using MeshCollection = std::unordered_map<int, MeshStorage>;

	MeshCollection m_meshStorages;

	auto findNode(auto this&& self, const path_t& path)
		-> decltype(&self.rootNode.value())
	{
		if (!self.rootNode.has_value())
			return nullptr;
		decltype(auto) currNode = &self.rootNode.value();
		for (auto& name : path) {
			auto nodeIt = currNode->childNodes.find(name);
			if (nodeIt == currNode->childNodes.end())
				return nullptr;
			currNode = nodeIt->second;
		}
		return currNode;
	}
};

} // dk::gfx
