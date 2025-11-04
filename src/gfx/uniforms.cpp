#include <devkit/gfx/uniforms.h>

#include <GL/glew.h>

#define DK_DECL_UNIFORM_SETTER(setter, type)                                                                    \
	template<>                                                                                                  \
	void details::gfx::setUniform<type>(unsigned program, const std::string& name, const type& value) {         \
		setter(glGetUniformLocation(program, name.c_str()), value);                                             \
	}                                                                                                           \
	/* end macro */

#define DK_DECL_UNIFORM_SETTER_CAST(setter, count, typeFrom, typeTo)                                            \
	template<>                                                                                                  \
	void details::gfx::setUniform<typeFrom>(unsigned program, const std::string& name, const typeFrom& value) { \
		setter(glGetUniformLocation(program, name.c_str()), count, (typeTo*)&value);                            \
	}                                                                                                           \
	/* end macro */

#define DK_DECL_UNIFORM_SETTER_MAT(setter, count, typeFrom, typeTo)                                             \
	template<>                                                                                                  \
	void details::gfx::setUniform<typeFrom>(unsigned program, const std::string& name, const typeFrom& value) { \
		setter(glGetUniformLocation(program, name.c_str()), count, GL_TRUE, (typeTo*)&(value[0]));              \
	}                                                                                                           \
	/* end macro */

DK_DECL_UNIFORM_SETTER(glUniform1i, bool);
DK_DECL_UNIFORM_SETTER(glUniform1i, int);
DK_DECL_UNIFORM_SETTER(glUniform1ui, unsigned int);
DK_DECL_UNIFORM_SETTER(glUniform1f, float);
DK_DECL_UNIFORM_SETTER(glUniform1d, double);

DK_DECL_UNIFORM_SETTER_CAST(glUniform2fv, 1, glm::vec2, float);
DK_DECL_UNIFORM_SETTER_CAST(glUniform3fv, 1, glm::vec3, float);
DK_DECL_UNIFORM_SETTER_CAST(glUniform4fv, 1, glm::vec4, float);

DK_DECL_UNIFORM_SETTER_MAT(glUniformMatrix3fv, 1, glm::mat3x3, float);
DK_DECL_UNIFORM_SETTER_MAT(glUniformMatrix4fv, 1, glm::mat4x4, float);
