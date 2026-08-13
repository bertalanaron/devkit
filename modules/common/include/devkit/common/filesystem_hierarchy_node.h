#pragma once
#include <devkit/common/utils.h>

namespace dk::common {

template <typename D>
class FSHierarchyNode {
public:
	FSHierarchyNode()                             = default;
	FSHierarchyNode(const FSHierarchyNode&)       = default;
	FSHierarchyNode(FSHierarchyNode&&)            = default;
	FSHierarchyNode& operator=(FSHierarchyNode&&) = default;

	auto children() const
	{
		return m_children | std::views::transform([&](const auto& pair) {
			return std::make_pair(pair.first, static_cast<const D*>(pair.second.get()));
		});
	}

	auto children()
	{
		return m_children | std::views::transform([&](auto& pair) {
			return std::make_pair(pair.first, static_cast<D*>(pair.second.get()));
		});
	}

	// @brief Resloves node using relative path. If node doesn't exist, returns nullptr. 
	// (E.g.: node[".."] returns parent node, node["/"])
	D* resolveRelativeNode(const std::filesystem::path& relativePath)
	{
		return resolveRelativeNodeImpl(relativePath);
	}

	// @brief Resloves node using relative path. If node doesn't exist, returns nullptr. 
	// (E.g.: node[".."] returns parent node, node["/"])
	const D* resolveRelativeNode(const std::filesystem::path& relativePath) const
	{
		return resolveRelativeNodeImpl(relativePath);
	}

	// @brief Inserts node as child if node is an immediate child of this node or if the node
	// is the child of an already added child node. 
	// @returns true if insertion was successful
	bool emplaceChild(const std::filesystem::path& relativePath, D&& node)
	{
		auto it = relativePath.begin();
		auto end = relativePath.end();

		if (it == end) 
			return false;

		auto const& head = *it;
		auto next = std::next(it);

		if (next == end) 
		{
			// immediate child
			if (m_children.contains(head)) 
				// already exists
				return false; 
			node.m_parent = this;
			m_children.emplace(head, std::make_unique<D>(std::move(node)));
			return true;
		} 
		else {
			// delegate deeper
			auto childIt = m_children.find(head);
			if (childIt == m_children.end()) 
				// parent path missing
				return false; 
			return childIt->second->emplaceChild(rebuildTail(next, end), std::move(node));
		}
	}

	void eraseChild(const std::filesystem::path& relativePath)
	{
		auto it = relativePath.begin();
		auto end = relativePath.end();

		if (it == end) 
			return;

		auto const& head = *it;
		auto next = std::next(it);

		if (next == end) 
		{
			// immediate child
			m_children.erase(head);
		} 
		else {
			auto childIt = m_children.find(head);
			if (childIt == m_children.end()) 
				// nothing to erase
				return; 
			childIt->second->eraseChild(rebuildTail(next, end));
		}
	}

protected:
	using ChildNodeMap = std::unordered_map<std::filesystem::path, copyable_unique_ptr<D>>;

	ChildNodeMap        m_children;
	FSHierarchyNode<D>* m_parent = nullptr;

	static std::filesystem::path rebuildTail(auto it, auto end)
	{
		// rebuild tail
		std::filesystem::path tail;
		for (; it != end; ++it) {
			tail /= *it;
		}
		return tail;
	}

	static std::filesystem::path rebuildParentOfTail(auto it, auto end)
	{
		// rebuild tail
		std::filesystem::path tail;
		for (; std::next(it) != end; ++it) {
			tail /= *it;
		}
		return tail;
	}

private:
	template <typename ThisT>
	using CCD = std::conditional_t<std::is_const_v<ThisT>, const D, D>;

	auto resolveRelativeNodeImpl(this auto&& self, const std::filesystem::path& relativePath)
		-> CCD<decltype(self)>*
	{
		decltype(auto) current = &self;
		for (auto const& part : relativePath) {
			if (part == ".") 
				continue;
			else if (part == "..") 
			{
				if (!current->m_parent)
					// already at root
					return nullptr; 
				current = current->m_parent;
			} 
			else if (part == "/") 
			{
				// climb to root
				while (current->m_parent)
					current = current->m_parent;
			} 
			else {
				auto it = current->m_children.find(part);
				if (it == current->m_children.end()) {
					return nullptr;
				}
				current = it->second.get();
			}
		}
		return static_cast<CCD<decltype(self)>*>(current);
	}
};

} // dk::common
