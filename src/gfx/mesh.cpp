#include <devkit/gfx/mesh.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

dk::gfx::Mesh dk::gfx::Mesh::create(VertexFlags flags)
{
	return Mesh(std::move(VertexBuffer::create(flags)), std::move(ElementBuffer()));
}

//dk::gfx::Mesh dk::gfx::Mesh::load(const std::string& path)
//{    
//    Assimp::Importer import;
//
//    const aiScene *scene = import.ReadFile(path, /*aiProcess_Triangulate | aiProcess_FlipUVs*/);	
//    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
//    {
//        spdlog::error("Assimp error: {}", import.GetErrorString());
//        return create<NullVertex>();
//    }
//
//    // process all the node's meshes (if any)
//    for(unsigned int i = 0; i < node->mNumMeshes; i++)
//    {
//        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
//        meshes.push_back(processMesh(mesh, scene));			
//    }
//    // then do the same for each of its children
//    for(unsigned int i = 0; i < node->mNumChildren; i++)
//    {
//        processNode(node->mChildren[i], scene);
//    }
//
//    return Mesh();
//}
