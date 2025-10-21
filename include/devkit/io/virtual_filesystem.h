//#pragma once
//#include <devkit/common/utils.h>
//
//namespace dk::io {
//
//class VirtualFSNodeBase {
//private:
//	using path_t = std::filesystem::path;
//	template <typename T>
//	using uptr_t = std::unique_ptr<T>;
//	using ChildLookup = std::unordered_map<path_t, uptr_t<VirtualFSNodeBase>>;
//
//public:
//	virtual bool isRecursive() const 
//	{ return false; }
//
//	virtual bool isLeaf() const 
//	{ return false; }
//
//	ChildLookup children;
//
//	template <typename T>
//		requires std::derived_from<T, VirtualFSNodeBase>
//	auto emplaceChild(const path_t& path, T&& child)
//	{
//		if (isRecursive() && !child.isLeaf())
//			throw std::runtime_error("Recursive node can only have leaf children");
//
//		// Update if exists
//		if (auto it = children.find(path); it != children.end())
//		{
//			it->second = std::make_unique<T>(std::move(child));
//			return it;
//		}
//
//		for (auto& existingChild : children)
//		{
//			// New node is child of existing child node
//			if (common::fs::is_parent(path, existingChild.first))
//			{
//				const auto relativePath = path.lexically_relative(existingChild.first);
//				return existingChild.second->emplaceChild(relativePath, std::move(child));
//			}
//
//			// New node is parent of existing child node
//			if (common::fs::is_parent(existingChild.first, path))
//			{
//				auto newNode = std::make_unique<T>(std::move(child));
//				auto existingMoved = std::move(existingMoved.second);
//
//				const auto relativePath = existingChild.first.lexically_relative(path);
//				dynamic_cast<VirtualFSNodeBase*>(newNode.get())->emplaceChild(relativePath, std::move(existingMoved));
//				existingChild.second = std::move(newNode);
//				return std::make_pair(existingChild, true);
//			}
//		}
//
//		return children.emplace(path, std::make_unique<T>(std::move(child)));
//	}
//
//	void removeChild(const path_t& path)
//	{
//		if (auto it = children.find(path); it != children.end())
//		{
//			children.erase(it);
//			return;
//		}
//
//		//for (auto& )
//	}
//
//	template <typename T>
//		requires std::derived_from<T, VirtualFSNodeBase>
//	T& access(const path_t& path)
//	{ return findForAccess(path)->second.get(); }
//
//	bool accessable(const path_t& path) const
//	{ return findForAccess(path) != children.end(); }
//
//private:
//	auto findForAccess(this auto&& self, const path_t& path)
//		-> decltype(self.children.begin())
//	{
//		decltype(auto) it = std::find_if(self.children.begin(), self.children.end(), 
//		[&](decltype(auto)& child) {
//			return std::filesystem::equivalent(path, child.first)
//				|| common::fs::is_parent(path, child.first);
//			});
//		return it;
//	}
//};
//
//} // dk::io
