#include <devkit/gfx/scene.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/Vertex.h>
#include <assimp/postprocess.h>

#define DK_CHECK_MESH_VERTEX_FLAG(aiGetter, name) if (mesh->aiGetter) flags = flags | dk::gfx::VertexFlags::name;

dk::gfx::VertexFlags getFlags(aiMesh* mesh, const aiScene* scene) 
{
    dk::gfx::VertexFlags flags = dk::gfx::VertexFlags::Position;
    DK_CHECK_MESH_VERTEX_FLAG(HasVertexColors(0) , Color0);
    DK_CHECK_MESH_VERTEX_FLAG(HasVertexColors(1) , Color1);
    DK_CHECK_MESH_VERTEX_FLAG(HasVertexColors(2) , Color2);
    DK_CHECK_MESH_VERTEX_FLAG(HasVertexColors(3) , Color3);
    DK_CHECK_MESH_VERTEX_FLAG(HasVertexColors(4) , Color4);
    DK_CHECK_MESH_VERTEX_FLAG(HasVertexColors(5) , Color5);
    DK_CHECK_MESH_VERTEX_FLAG(HasVertexColors(6) , Color6);
    DK_CHECK_MESH_VERTEX_FLAG(HasVertexColors(7) , Color7);
    DK_CHECK_MESH_VERTEX_FLAG(HasNormals()       , Normals);
    DK_CHECK_MESH_VERTEX_FLAG(HasTextureCoords(0), TexCoord);
    DK_CHECK_MESH_VERTEX_FLAG(HasBones()         , Bones);
    return flags;
}

std::vector<uint8_t> getVertex(size_t completeSize, dk::gfx::VertexFlags flags, aiMesh* mesh, unsigned int vertexIndex) 
{
    std::vector<uint8_t> result;
    result.resize(completeSize);
    // Position
    size_t i = 0;
    reinterpret_cast<glm::vec3&>(result.at(i)) = glm::vec3(mesh->mVertices[vertexIndex].x, mesh->mVertices[vertexIndex].y, mesh->mVertices[vertexIndex].z);
    i += sizeof(glm::vec3);
    // Color0
    if ((unsigned)flags & (unsigned)dk::gfx::VertexFlags::Color0) {
        reinterpret_cast<glm::vec4&>(result.at(i)) = glm::vec4(mesh->mColors[0][vertexIndex].r, mesh->mColors[0][vertexIndex].g, mesh->mColors[0][vertexIndex].b, mesh->mColors[0][vertexIndex].a);
        i += sizeof(glm::vec4);
    }
    // TODO:
    // ...
    // Normals
    if ((unsigned)flags & (unsigned)dk::gfx::VertexFlags::Normals) {
        reinterpret_cast<glm::vec3&>(result.at(i)) = glm::vec3(mesh->mNormals[vertexIndex].x, mesh->mNormals[vertexIndex].y, mesh->mNormals[vertexIndex].z);
        i += sizeof(glm::vec3);
    }
    // TexCoords
    if ((unsigned)flags & (unsigned)dk::gfx::VertexFlags::TexCoord) {
        reinterpret_cast<glm::vec2&>(result.at(i)) = glm::vec2(mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y);
        i += sizeof(glm::vec2);
    }
    return result;
}

dk::gfx::Mesh dk::gfx::Scene::processMesh(void* _mesh, const void* _scene)
{
    auto mesh  = reinterpret_cast<aiMesh*>(_mesh);
    auto scene = reinterpret_cast<const aiScene*>(_scene);

    VertexFlags flags = getFlags(mesh, scene);
    Mesh result = Mesh::create(flags);

    // Push vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        result.vertices().push(getVertex(result.vertices().vertexAttributes()->size(), flags, mesh, i));

    // Push indices
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            result.indices().push(face.mIndices[j]);
    }

    return result;
}

void dk::gfx::Scene::processNode(void* _node, const void* _scene)
{
    auto node  = reinterpret_cast<aiNode*>(_node);
    auto scene = reinterpret_cast<const aiScene*>(_scene);

    // Process meshes
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        m_meshes.push_back(processMesh(mesh, scene));			
    }

    // Process child nodes
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

dk::gfx::Scene dk::gfx::Scene::load(const std::string& path)
{
    Scene result;

    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);	

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        spdlog::error("Failed to load scene from {}", path);
        spdlog::error("Assimp error: {}", import.GetErrorString());
        return result;
    }

    result.m_directory = path.substr(0, path.find_last_of('/'));

    result.processNode(scene->mRootNode, scene);
    
    return result;
}
