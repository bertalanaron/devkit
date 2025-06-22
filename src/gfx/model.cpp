#include <devkit/gfx/vertex.h>
#include <devkit/gfx/shader.h>

#include <GL/glew.h>


//template<>
//gfx::GLuint toGlProperty<dk::gfx::properties::texture::min_filter>()
//{ return GL_TEXTURE_MIN_FILTER; }
//
//template<>
//gfx::GLuint toGlProperty<dk::gfx::properties::texture::mag_filter>()
//{ return GL_TEXTURE_MAG_FILTER; }
//
//template <>
//gfx::GLuint toGlProperty<dk::gfx::properties::shader::depth_func>()
//{ return GL_DEPTH_FUNC; }


//template <>
//gl_property_setter_t toGlPropertySetter<>(dk::gfx::properties::shader::depth_func value) 
//{ 
//	glDepthFunc((GLuint)value);
//}

//}



//template <>
//details::gfx::GLuint details::gfx::genResource<details::gfx::gl_resource_type::VertexBuffer>() 
//{ 
//	details::gfx::GLuint handle;
//	glGenBuffers(GL_ARRAY_BUFFER, &handle);
//	return handle;
//}
//
//template <>
//void details::gfx::deleteResource<details::gfx::gl_resource_type::VertexBuffer>(details::gfx::GLuint handle) 
//{ 
//	glDeleteBuffers(1, &handle);
//}
//
//template <>
//details::gfx::GLuint details::gfx::genResource<details::gfx::gl_resource_type::Shader>() 
//{ 
//	return glCreateProgram();
//}
//
//template <>
//void details::gfx::deleteResource<details::gfx::gl_resource_type::Shader>(details::gfx::GLuint handle) 
//{ 
//	glDeleteProgram(handle);
//}
