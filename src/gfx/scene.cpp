#include <devkit/gfx/scene.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/Vertex.h>
#include <assimp/postprocess.h>

glm::mat4 toGlm(const aiMatrix4x4 & mat)
{
    return glm::mat4(
        mat.a1, mat.b1, mat.c1, mat.d1,
        mat.a2, mat.b2, mat.c2, mat.d2,
        mat.a3, mat.b3, mat.c3, mat.d3,
        mat.a4, mat.b4, mat.c4, mat.d4 );
}

std::string toStd(const aiString& string)
{
    return string.C_Str();
}

#pragma region AIMESH_PARSE_MACRO_MAGIC
#define __DK_AIMESH_PARSE_LUT(F, ...)                                                                             \
    /* Other Args           | Enable | ai Name           | Members           | Exists                          */ \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mVertices         , __DK_MEMBERS_XYZ  , true                             ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mColors[0]        , __DK_MEMBERS_RGBA , mesh->HasVertexColors(0)         ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mColors[1]        , __DK_MEMBERS_RGBA , mesh->HasVertexColors(1)         ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mColors[2]        , __DK_MEMBERS_RGBA , mesh->HasVertexColors(2)         ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mColors[3]        , __DK_MEMBERS_RGBA , mesh->HasVertexColors(3)         ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mColors[4]        , __DK_MEMBERS_RGBA , mesh->HasVertexColors(4)         ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mColors[5]        , __DK_MEMBERS_RGBA , mesh->HasVertexColors(5)         ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mColors[6]        , __DK_MEMBERS_RGBA , mesh->HasVertexColors(6)         ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mColors[7]        , __DK_MEMBERS_RGBA , mesh->HasVertexColors(7)         ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mNormals          , __DK_MEMBERS_XYZ  , mesh->HasNormals()               ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mTextureCoords[0] , __DK_MEMBERS_XY   , mesh->HasTextureCoords(0)        ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mTangents         , __DK_MEMBERS_XYZ  , mesh->HasTangentsAndBitangents() ) \
    F( __VA_ARGS__ __VA_OPT__(,)   1 , mBitangents       , __DK_MEMBERS_XYZ  , mesh->HasTangentsAndBitangents() ) \
    F( __VA_ARGS__ __VA_OPT__(,)   0 , -                 , -                 , mesh->HasBones()                 ) \
    /* end table */

#define __DK_AIMATERIAL_PARSE_LUT(F, ...)                \
    F( __VA_ARGS__ __VA_OPT__(,) aiTextureType_DIFFUSE ) \
    /* end table */

#define __DK_AIMESH_GETENABLE(index, name, type, enable, aiName, MEMBERS, exists) enable
// @brief Eval macro with each vertex flag right joined with it's aimesh parse table entry
#define __DK_FOREACH_JOINED_AIMESHLUT_VERTEXFLAGS(F) __DK_JOIN_WITH_LUT(__DK_AIMESH_PARSE_LUT, DK_VERTEXFLAGS_TABLE, F)
// @brief Eval macro only on enabled enties
#define __DK_CALL_ON_ENABLED_JOINED_AIMESHVERTEXFLAGS(F, ...) DK_OPT(__DK_AIMESH_GETENABLE(__VA_ARGS__), F(__VA_ARGS__))

#define __DK_AIMESH_PARSE_FLAG(index, name, type, enable, aiName, MEMBERS, exists) \
    if (exists) { flags = flags | dk::gfx::VertexFlags::name; }                    \
    /* end of macro */
// @brief if (mesh->HasFlag()) { flags |= dk::gfx::VertexFlags::Flag; }
#define __DK_AIMESH_TRY_PARSE_FLAGS(...) __DK_AIMESH_PARSE_FLAG(__VA_ARGS__)

#define __DK_AIMESH_PARSE_VERTEX_COMPONENT(type, aiName, index, member, notLast)     \
    mesh->aiName[vertexIndex].member __DK_CONCAT2_DEFERRED(__DK_OPT_COMMA_, notLast) \
    /* end of macro */
#define __DK_AIMESH_PARSE_VERTEX(index, name, type, enable, aiName, MEMBERS, exists) \
    if ((unsigned)flags & (unsigned)dk::gfx::VertexFlags::name) {                    \
        vertex.push_back(type(MEMBERS(__DK_AIMESH_PARSE_VERTEX_COMPONENT, type, aiName, index))); \
    }                                                                                \
    /* end of macro */
#define __DK_AIMESH_TRY_PARSE_VERTEX(...) __DK_CALL_ON_ENABLED_JOINED_AIMESHVERTEXFLAGS(__DK_AIMESH_PARSE_VERTEX, __VA_ARGS__)
#pragma endregion

dk::gfx::VertexFlags parseFlags(aiMesh* mesh)
{
    dk::gfx::VertexFlags flags = dk::gfx::VertexFlags::Position;
    __DK_FOREACH_JOINED_AIMESHLUT_VERTEXFLAGS(__DK_AIMESH_TRY_PARSE_FLAGS)
        return flags;
}

dk::common::typeless_builder parseVertex(
    size_t               completeSize, 
    dk::gfx::VertexFlags flags, 
    aiMesh*              mesh, 
    unsigned int         vertexIndex)
{
    dk::common::typeless_builder vertex(completeSize);

    __DK_FOREACH_JOINED_AIMESHLUT_VERTEXFLAGS(__DK_AIMESH_TRY_PARSE_VERTEX)

    // TODO: parse bones

    return vertex;
}

#pragma region PARSE MATERIAL MACRO MAGIC
#define __DK_AIMATERIAL_PARSE_LUT(F, ...) \
    F( __VA_ARGS__ __VA_OPT__(,) aiTextureType_BASE_COLOR        ) \
    F( __VA_ARGS__ __VA_OPT__(,) aiTextureType_DIFFUSE           ) \
    F( __VA_ARGS__ __VA_OPT__(,) aiTextureType_NORMALS           ) \
    F( __VA_ARGS__ __VA_OPT__(,) aiTextureType_SPECULAR          ) \
    F( __VA_ARGS__ __VA_OPT__(,) aiTextureType_HEIGHT            ) \
    F( __VA_ARGS__ __VA_OPT__(,) aiTextureType_EMISSIVE          ) \
    F( __VA_ARGS__ __VA_OPT__(,) aiTextureType_METALNESS         ) \
    F( __VA_ARGS__ __VA_OPT__(,) aiTextureType_DIFFUSE_ROUGHNESS ) \
    /* end of table */
#define __DK_FOREACH_JOINED_AIMATERIALLUT_TEXTURETYPE(F) __DK_JOIN_WITH_LUT(__DK_AIMATERIAL_PARSE_LUT, __DK_TABLE_TEXTURETYPE, F)
#define __DK_PARSE_TEXTURE_TYPE(index, type, uniformName, aiName)                   \
    if (auto count = material->GetTextureCount(aiName); count > 0) {                \
        for (int i = 0; i < count; ++i) {                                           \
            aiString aiStr;                                                         \
            material->GetTexture(aiName, i, &aiStr);                                \
            textures.at((unsigned)dk::gfx::Material::type).push_back(toStd(aiStr)); \
        }                                                                           \
    }                                                                               \
    /* end of macro */
#pragma endregion

dk::gfx::Material parseMaterials(aiMesh* mesh, const aiScene* scene)
{
    dk::gfx::Material::TextureCollection textures;
    auto material = scene->mMaterials[mesh->mMaterialIndex];

    __DK_FOREACH_JOINED_AIMATERIALLUT_TEXTURETYPE(__DK_PARSE_TEXTURE_TYPE)

    return dk::gfx::Material(std::move(textures));
}

dk::gfx::Scene::MeshFactory parseMesh(aiMesh* mesh, const aiScene* scene)
{
    // Parse flags from aiMesh
    const auto    vertexFlags = parseFlags(mesh);
    // Initialize result mesh
    dk::gfx::Mesh result(vertexFlags);
    const auto    vertexSize = result.vertices.vertexAttributes()->size();

    // Parse vertices
    for (int i = 0; i < mesh->mNumVertices; ++i)
    {
        const auto parsedVertex = parseVertex(vertexSize, vertexFlags, mesh, i);
        result.vertices.modify().push_back(parsedVertex.get());
    }

    // Parse indices (faces)
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            result.indices.push(face.mIndices[j]);
    }

    // Parse material
    result.material.emplace(parseMaterials(mesh, scene));

    return dk::gfx::Scene::MeshFactory(std::move(result), vertexFlags);
}

dk::gfx::Scene::SceneNode parseNode(
    dk::gfx::Scene&  parsedScene,
    const aiNode*    node, 
    const aiScene*   scene, 
    const glm::mat4& parentTransform)
{
    // Parse mesh indices
    std::vector<unsigned> meshIndices;
    for (int i = 0; i < node->mNumMeshes; ++i)
        meshIndices.push_back(node->mMeshes[i]);

    dk::gfx::Scene::SceneNode result(parsedScene, std::move(meshIndices));

    // Calculate transform
    result.transform = parentTransform * toGlm(node->mTransformation);

    // Set name
    result.name = toStd(node->mName);

    // Parse child nodes
    for (int i = 0; i < node->mNumChildren; ++i)
    {
        auto childName = toStd(node->mChildren[i]->mName);
        result.emplaceChild(childName, parseNode(parsedScene, node->mChildren[i], scene, result.transform));
    }

    return result;
}

dk::gfx::Scene::Scene(const Scene& other)
{
    throw std::runtime_error("Not implemented");
}

dk::gfx::Scene::Scene(const std::filesystem::path& path)
{
    // Try load scene from file
    Assimp::Importer import;
    const auto       sceneFlags = aiProcess_Triangulate | aiProcess_FlipUVs/* | aiProcess_CalcTangentSpace*/;
    const aiScene*   scene      = import.ReadFile(path.string(), sceneFlags);

    // Verify scene
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        spdlog::error("Failed to load scene from {}", path.string());
        spdlog::error("Assimp error: {}", import.GetErrorString());
        return;
    }

    // Parse meshes
    for (int i = 0; i < scene->mNumMeshes; ++i)
        m_meshFactories.emplace_back(std::move(parseMesh(scene->mMeshes[i], scene)));

    // Parse nodes
    m_root.emplace(parseNode(*this, scene->mRootNode, scene, glm::identity<glm::mat4>()));
}
