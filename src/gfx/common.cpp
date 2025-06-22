#include <devkit/gfx/common.h>

#include <GL/glew.h>

unsigned int details::gfx::toUnderlying(dk::gfx::Primitive primitive)
{
    switch (primitive)
    {
    case dk::gfx::Primitive::Points:    return GL_POINTS;
    case dk::gfx::Primitive::Lines:     return GL_LINES;
    case dk::gfx::Primitive::Triangles: return GL_TRIANGLES;
    default:
        break;
    }
}

template <>
details::gfx::GLType details::gfx::GLType::get<float>() 
{ return GLType{ GL_FLOAT, sizeof(float), 1 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<double>() 
{ return GLType{ GL_DOUBLE, sizeof(double), 1 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<char>() 
{ return GLType{ GL_BYTE, sizeof(char), 1 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<unsigned char>() 
{ return GLType{ GL_UNSIGNED_BYTE, sizeof(unsigned char), 1 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<std::byte>() 
{ return GLType{ GL_UNSIGNED_BYTE, sizeof(std::byte), 1 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<short>() 
{ return GLType{ GL_SHORT, sizeof(short), 1 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<unsigned short>() 
{ return GLType{ GL_UNSIGNED_SHORT, sizeof(unsigned short), 1 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<int>() 
{ return GLType{ GL_INT, sizeof(int), 1 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<unsigned int>() 
{ return GLType{ GL_UNSIGNED_INT, sizeof(unsigned int), 1 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<glm::vec2>() 
{ return GLType{ GL_FLOAT, sizeof(float) * 2, 2 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<glm::vec3>() 
{ return GLType{ GL_FLOAT, sizeof(float) * 3, 3 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<glm::vec4>() 
{ return GLType{ GL_FLOAT, sizeof(float) * 4, 4 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<glm::mat3>() 
{ return GLType{ GL_FLOAT, sizeof(float) * 3, 3, 3 }; }

template <>
details::gfx::GLType details::gfx::GLType::get<glm::mat4>() 
{ return GLType{ GL_FLOAT, sizeof(float) * 4, 4, 4 }; }
