#include <devkit/graphics.h>
#include "graphics_includes.h"

using namespace NS_DEVKIT;

template <>
GLType GLType::get<float>() {
	return GLType{ GL_FLOAT, sizeof(float), 1 };
}

template <>
GLType GLType::get<double>() {
	return GLType{ GL_DOUBLE, sizeof(double), 1 };
}

template <>
GLType GLType::get<char>() {
	return GLType{ GL_BYTE, sizeof(char), 1 };
}

template <>
GLType GLType::get<unsigned char>() {
	return GLType{ GL_UNSIGNED_BYTE, sizeof(unsigned char), 1 };
}

template <>
GLType GLType::get<std::byte>() {
	return GLType{ GL_UNSIGNED_BYTE, sizeof(std::byte), 1 };
}

template <>
GLType GLType::get<short>() {
	return GLType{ GL_SHORT, sizeof(short), 1 };
}

template <>
GLType GLType::get<unsigned short>() {
	return GLType{ GL_UNSIGNED_SHORT, sizeof(unsigned short), 1 };
}

template <>
GLType GLType::get<int>() {
	return GLType{ GL_INT, sizeof(int), 1 };
}

template <>
GLType GLType::get<unsigned int>() {
	return GLType{ GL_UNSIGNED_INT, sizeof(unsigned int), 1 };
}

template <>
GLType GLType::get<glm::vec2>() {
	return GLType{ GL_FLOAT, sizeof(float) * 2, 2 };
}

template <>
GLType GLType::get<glm::vec3>() {
	return GLType{ GL_FLOAT, sizeof(float) * 3, 3 };
}

template <>
GLType GLType::get<glm::vec4>() {
	return GLType{ GL_FLOAT, sizeof(float) * 4, 4 };
}
